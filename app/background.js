var enabled = false;
var poolSetting, paramSetting;
var freqSet, voltSet;

var pools = [], avalons = [];
var jobQueue = [
	{thisId: -1, value: []},
	{thisId: -1, value: []},
	{thisId: -1, value: []},
];
var thread = new Worker("thread.js");
var workQueue = {
	value: [],
	shift: function() {
		if (workQueue.value.length < 1000)
			thread.postMessage({info: "resume"});
		return this.value.shift();
	},
	push: function(data) {
		if (workQueue.value.length >= 1023)
			thread.postMessage({info: "pause"});
		return this.value.push(data);
	},
	init: function() {
		this.value = [];
	},
};
var activePool = Infinity; // Bigger than bigger. Low id gets high priority.

var hashrates = [];
var totalHashrate = Array.apply(null, new Array(721)).map(Number.prototype.valueOf, 0);

chrome.app.runtime.onLaunched.addListener(function() {
	chrome.storage.local.get("pool", function(result) {
		poolSetting = result.pool || [];
	});
	chrome.storage.local.get("param", function(result) {
		paramSetting = result.param || [];
		freqSet = paramSetting.freqSet;
		voltSet = paramSetting.voltSet;
	});

	chrome.app.window.create("index.html", {
		innerBounds: {
			width: 1000,
			height: 700,
			minWidth: 1000,
			minHeight: 700
		},
		id: "Avalon miner"
	});

	chrome.runtime.onMessage.addListener(function(msg, sender) {
		var i;
		if (sender.url.indexOf("index.html") === -1)
			return;
		switch (msg.type) {
		case "ready":
			prelude();
			break;
		case "start":
			main();
			break;
		case "setting":
			if (msg.pool !== undefined) {
				for (i = 0; i < pools.length; i++) {
					pools[i].disconnect();
					delete(pools[i]);
				}
				poolSetting = msg.pool;
				if (enabled) {
					thread.postMessage({info: "pause"});
					activePool = Infinity;
					for (i = 0; i < poolSetting.length; i++) {
						var p = poolSetting[i];
						pools[i] = new Pool(i, p.address, p.port, p.username, p.password);
						pools[i].onJob.addListener(jobHandler);
						pools[i].onError.addListener(errorHandler);
						pools[i].connect();
					}
				}
			}
			if (msg.param !== undefined) {
				paramSetting = msg.param;
				for (var avalon in avalons) {
					if (avalon === undefined)
						continue;
					avalon.setVoltage(volt);
					avalon.setVoltage(freqs);
				}
			}
			chrome.storage.local.set({
				pool: poolSetting,
				param: paramSetting,
			});
			break;
		}
	});
});

var prelude = function() {
	chrome.runtime.sendMessage({
		type: "setting",
		pool: poolSetting,
		param: paramSetting,
	});

	scanDevices();

	chrome.hid.onDeviceAdded.addListener(function(device) {
		var id = device.deviceId;
		avalons[id] = new Avalon(device, workQueue, voltSet, freqSet);
		chrome.runtime.sendMessage({
			type: "device",
			deviceId: id,
			deviceType: avalons[id].deviceType,
		});
		avalons[id].onDetect.addListener(detectHandler);
		avalons[id].onNonce.addListener(nonceHandler);
		avalons[id].onStatus.addListener(statusHandler);
		hashrates[id] = hashrates[id] ||
			Array.apply(null, new Array(721)).map(Number.prototype.valueOf, 0);
		if (enabled)
			avalons[id].connect();
	});

	chrome.hid.onDeviceRemoved.addListener(function(deviceId) {
		for (var idx in avalons) {
			var avalon = avalons[idx];
			if (avalon.deviceId === deviceId) {
				avalon.stop();
				delete(avalons[idx]);
				delete(hashrates[idx]);
				chrome.runtime.sendMessage({type: "delete", deviceId: deviceId});
				return;
			}
		}
	});

};

var main = function() {
	var i, pool, avalon;
	enabled = true;
	activePool = Infinity;
	for (i = 0; i < poolSetting.length; i++) {
		var p = poolSetting[i];
		pools[i] = new Pool(i, p.address, p.port, p.username, p.password);
	}

	thread.onmessage = function(work) {
		workQueue.push(work.data);
	};

	chrome.sockets.tcp.onReceive.addListener(function(info) {
		for (var pool of pools)
			if (pool !== undefined && info.socketId === pool.socketId) {
				pool.receive(info.data);
				return;
			}
	});

	chrome.sockets.tcp.onReceiveError.addListener(function(info) {
		for (var pool of pools)
			if (pool !== undefined && info.socketId === pool.socketId) {
				if (pool.id === activePool) {
					thread.postMessage({info: "pause"});
					activePool++;
					workQueue.init();
					if (pool.id !== 3) {
						var jq = jobQueue[activePool];
						thread.postMessage({
							info: "newJob",
							job: jq.value[jq.thisId],
							jqId: jq.thisId,
						});
					}
				}
				chrome.runtime.sendMessage({
					info: "Failed",
					type: "pool",
					poolId: pool.id,
				});
				pool.connect();
				return;
			}
	});

	for (pool of pools) {
		pool.onJob.addListener(jobHandler);
		pool.onError.addListener(errorHandler);
		pool.connect();
	}

	calcHashrate();

	for (avalon of avalons)
		if (avalon !== undefined)
			avalon.connect();
};

var jobHandler = function(job) {
	var poolId = job.poolId;
	var jq = jobQueue[poolId];
	jq.thisId = (jq.thisId + 1) % 256;
	jq.value[jq.thisId] = job;
	if (poolId < activePool) {
		if (activePool < 3)
			chrome.runtime.sendMessage({
				info: "Inactive",
				type: "pool",
				poolId: activePool,
			});
		activePool = poolId;
		workQueue.init();
		chrome.runtime.sendMessage({
			info: "Active",
			type: "pool",
			poolId: poolId,
		});
	} else if (poolId > activePool) {
		chrome.runtime.sendMessage({
			info: "Inactive",
			type: "pool",
			poolId: poolId,
		});
		return;
	} else
		workQueue.init();
	thread.postMessage({info: "newJob", job: job, jqId: jq.thisId});
};

var errorHandler = function(poolId) {
	chrome.runtime.sendMessage({
		info: "Failed",
		type: "pool",
		poolId: poolId,
	});
};

var detectHandler = function(info) {
	var dna = info.dna;
	var version = info.version;
	var id = info.deviceId;
	chrome.runtime.sendMessage(
		{type: "status", deviceId: id, version: version, dna: dna}
	);
	avalons[id].run();
};

var nonceHandler = function(info) {
	if (jobQueue[info.poolId] === undefined)
		return;
	var job = jobQueue[info.poolId].value[info.jqId];
	if (job === undefined)
		return;
	var ntime = info.ntime + parseInt(job.ntime, 16);
	switch (utils.varifyWork(job, info.nonce2, ntime, info.nonce)) {
		case 2:
			// hard ware error
			return;
		case 0:
			pools[info.poolId].submit(
				job.jobId,
				utils.uInt2LeHex(info.nonce2, job.nonce2Size),
				ntime.toString(16),
				utils.uInt2LeHex(info.nonce, 4)
			);
			/* falls through */
		case 1:
			hashrates[info.deviceId][0]++;
			totalHashrate[0]++;
			return;
	}
};

var statusHandler = function(stats) {
	stats.type = "status";
	chrome.runtime.sendMessage(stats);
};

var scanDevices = function() {
	chrome.hid.getDevices(Avalon.FILTERS, function(devices) {
		if (chrome.runtime.lastError) {
			return;
		}
		for (var device of devices) {
			var id = device.deviceId;
			avalons[id] = new Avalon(device, workQueue, voltSet, freqSet);
			chrome.runtime.sendMessage({
				type: "device",
				deviceId: id,
				deviceType: avalons[id].deviceType,
			});
			avalons[id].onDetect.addListener(detectHandler);
			avalons[id].onNonce.addListener(nonceHandler);
			avalons[id].onStatus.addListener(statusHandler);
			hashrates[id] = hashrates[id] ||
				Array.apply(null, new Array(721)).map(Number.prototype.valueOf, 0);
		}
	});
};

var calcHashrate = function() {
	(function loop() {
		var hashrate = [];
		var h;
		for (var i in hashrates) {
			h = hashrates[i];
			if (h !== undefined) {
				h.unshift(0);
				hashrate.push({
					deviceId: i,
					hs1h: utils.arraySum(h.slice(1)) / 3600 * 4294967296,
					hs15m: utils.arraySum(h.slice(1, 181)) / 900 * 4294967296,
					hs5m: utils.arraySum(h.slice(1, 61)) / 300 * 4294967296,
					hs1m: utils.arraySum(h.slice(1, 13)) / 60 * 4294967296,
					hs15s: utils.arraySum(h.slice(1, 4))/ 15 * 4294967296,
					hs5s: h[1] / 5 * 4294967296,
				});
				h.pop();
			}
		}
		h = totalHashrate;
		h.unshift(0);
		hashrate.push({
			deviceId: null,
			hs1h: utils.arraySum(h.slice(1)) / 3600 * 4294967296,
			hs15m: utils.arraySum(h.slice(1, 181)) / 900 * 4294967296,
			hs5m: utils.arraySum(h.slice(1, 61)) / 300 * 4294967296,
			hs1m: utils.arraySum(h.slice(1, 13)) / 60 * 4294967296,
			hs15s: utils.arraySum(h.slice(1, 4))/ 15 * 4294967296,
			hs5s: h[1] / 5 * 4294967296,
		});
		h.pop();
		chrome.runtime.sendMessage({
			type: "hashrate",
			hashrate: hashrate,
		});
		setTimeout(loop, 5000);
	})();
};
