var Miner = function() {
	// Events
	this.onNewNano = new MinerEvent();
	this.onNanoDeleted = new MinerEvent();
	this.onNanoConnected = new MinerEvent();
	this.onNanoDetected = new MinerEvent();
	this.onNewStatus = new MinerEvent();
	this.onPoolSubscribed = new MinerEvent();
	this.onPoolAuthorized = new MinerEvent();
	this.onHashrate = new MinerEvent();

	this._JOB_BUFFER_SIZE = 256;
	this._WORK_BUFFER_SIZE = 1024;

	this._jobId = [];
	this._poolId = 0;

	this._jobs = [];
	this._works = [];

	this._nanos = [];
	this._pools = [];
	this._hashrates = [];
	this._total_hashrate = Array.apply(null, new Array(3600)).map(Number.prototype.valueOf, 0);
	var miner = this;

	this.scanNano();

	chrome.hid.onDeviceAdded.addListener(function(device) {
		miner.newNano = {
			nano: new Nano(device, miner),
			nanoId: device.deviceId
		};
	});
	chrome.hid.onDeviceRemoved.addListener(function(deviceId) {
		miner.nanoDeleted = deviceId;
	});
	chrome.sockets.tcp.onReceive.addListener(function(info) {
		for (var pool of miner._pools)
			if (pool !== undefined && info.socketId === pool.socketId) {
				pool.download(info);
				return;
			}
	});

	this._thread = [];
	this._thread_pause = [];

	this._stop = false;
	(function loop() {
		var hashrate = [];
		var h;
		for (var i in miner._hashrates) {
			h = miner._hashrates[i];
			if (h !== undefined) {
				h.unshift(0);
				hashrate.push({
					nanoId: i,
					hs1h: arraySum(h.slice(1)) / 3600 * 4294967296,
					hs15m: arraySum(h.slice(1, 901)) / 900 * 4294967296,
					hs5m: arraySum(h.slice(1, 301)) / 300 * 4294967296,
					hs1m: arraySum(h.slice(1, 61)) / 60 * 4294967296,
					hs15s: arraySum(h.slice(1, 16))/ 15 * 4294967296,
					hs5s: arraySum(h.slice(1, 6)) / 5 * 4294967296,
					hs1s: h[1] * 4294.967296
				});
				h.pop();
			}
		}
		h = miner._total_hashrate;
		h.unshift(0);
		hashrate.push({
			nanoId: null,
			hs1h: arraySum(h.slice(1)) / 3600 * 4294967296,
			hs15m: arraySum(h.slice(1, 901)) / 900 * 4294967296,
			hs5m: arraySum(h.slice(1, 301)) / 300 * 4294967296,
			hs1m: arraySum(h.slice(1, 61)) / 60 * 4294967296,
			hs15s: arraySum(h.slice(1, 16))/ 15 * 4294967296,
			hs5s: arraySum(h.slice(1, 6)) / 5 * 4294967296,
			hs1s: h[1] * 4294.967296
		});
		h.pop();
		miner.hashrate = hashrate;
		if (!miner._stop)
			setTimeout(loop, 1000);
	})();
};

Miner.prototype.__defineSetter__("newJob", function(job) {
	var poolId = job.poolId;
	this._jobId[poolId] = (this._jobId[poolId] + 1) % this._JOB_BUFFER_SIZE;
	this._jobs[poolId][this._jobId] = job;

	// clear work buffer
	this._works[poolId] = [];
	this._thread_pause[poolId] = false;

	//console.log("[Miner] New Job:");
	this._thread[poolId].postMessage({
		job: job,
		jobId: this._jobId[poolId],
		poolId: poolId
	});
});

Miner.prototype.__defineSetter__("newNano", function(msg) {
	// msg: {nanoId, nano}
	this.log("info", "New Nano: %d", msg.nanoId);
	this._nanos[msg.nanoId] = msg.nano;
	this._hashrates[msg.nanoId] = this._hashrates[msg.nanoId] ||
		Array.apply(null, new Array(3600)).map(Number.prototype.valueOf, 0);
	msg.nano.connect();

	this.onNewNano.fire(msg);
});

Miner.prototype.__defineSetter__("nanoDeleted", function(nanoId) {
	this.log("info", "Nano Deleted: %d", nanoId);
	this._nanos[nanoId].stop();
	delete(this._nanos[nanoId]);
	delete(this._hashrates[nanoId]);
	this.onNanoDeleted.fire(nanoId);
});

Miner.prototype.__defineSetter__("nanoConnected", function(msg) {
	// msg: {nanoId, success}
	if (msg.success)
		this._nanos[msg.nanoId].detect();
	this.onNanoConnected.fire(msg);
});

Miner.prototype.__defineSetter__("nanoDetected", function(msg) {
	// msg: {nanoId, success, version}
	if (msg.success)
		// TODO: MINI runs at argument of 4 and NANO of 1
		//       Check PID
		this._nanos[msg.nanoId].run(4);
	this.onNanoDetected.fire(msg);
});

Miner.prototype.__defineSetter__("poolSubscribed", function(msg) {
	// msg: {poolId, success}
	this.onPoolSubscribed.fire(msg);
});

Miner.prototype.__defineSetter__("poolAuthorized", function(msg) {
	// msg: {poolId, success}
	this.onPoolAuthorized.fire(msg);
});

Miner.prototype.__defineSetter__("newNonce", function(msg) {
	// msg: {nanoId, nonce, nonce2, jobId, poolId, sub_ntime}
	if (this._jobs[msg.poolId] === undefined)
		return;
	var job = this._jobs[msg.poolId][msg.jobId];
	if (job === undefined)
		// Some false nonces without valid jobId during the first several runs.
		return;
	var ntime = msg.ntime + parseInt(job.ntime, 16);

	switch (varifyWork(job, msg.nonce2, ntime, msg.nonce)) {
		case 2:
			// hard ware error
			return;
		case 0:
			this._pools[msg.poolId].submit(
				job.job_id,
				uInt2LeHex(msg.nonce2, job.nonce2_size),
				ntime.toString(16),
				uInt2LeHex(msg.nonce, 4)
			);
			/* falls through */
		case 1:
			this._hashrates[msg.nanoId][0]++;
			this._total_hashrate[0]++;
			return;
	}
});

Miner.prototype.__defineSetter__("newStatus", function(msg) {
	// msg: {nanoId, stat}
	// TODO: update status
	this.onNewStatus.fire(msg);
});

Miner.prototype.__defineSetter__("hashrate", function(hashrate) {
	this.onHashrate.fire(hashrate);
});

Miner.prototype.__defineGetter__("getWork", function() {
	if (this._active_pool === undefined)
		return undefined;
	var work = this._works[this._active_pool].shift();
	if (this._thread_pause[this._active_pool] &&
		(this._works[this._active_pool].length < this._WORK_BUFFER_SIZE - 10)) {
			this._thread[this._active_pool].postMessage("resume");
			this._thread_pause[this._active_pool] = false;
	}
	return work;
});

Miner.prototype.setPool = function(poolInfo, poolId) {
	if (this._pools[poolId] !== undefined)
		this._pools[poolId].disconnect();
	poolInfo.id = poolId;
	this._jobId[poolId] = -1;
	this._jobs[poolId] = [];
	this._works[poolId] = [];
	this._pools[poolId] = new Pool(poolInfo, this);

	this._thread[poolId] = new Worker("thread.js");
	this._thread[poolId].onmessage = function(work) {
		miner._works[poolId].push(work.data);
		if (miner._works[poolId].length >= miner._WORK_BUFFER_SIZE) {
			miner._thread[poolId].postMessage("pause");
			miner._thread_pause[poolId] = true;
		}
	};

	this._pools[poolId].run();
	this._active_pool = this._active_pool || poolId;
};

Miner.prototype.deletePool = function(poolId) {
	this._pools[poolId].disconnect();
	delete(this._works[poolId]);
	delete(this._thread[poolId]);
	delete(this._thread_pause[poolId]);
	delete(this._jobId[poolId]);
	delete(this._pools[poolId]);

	// TODO: Change this._active_pool
};

Miner.prototype.scanNano = function() {
	var miner = this;
	chrome.hid.getDevices(FILTERS, function(devices) {
		if (chrome.runtime.lastError) {
			console.error(chrome.runtime.lastError.message);
			return;
		}
		for (var device of devices) {
			miner.newNano = {
				nano: new Nano(device, miner),
				nanoId: device.deviceId
			};
		}
	});
};

Miner.prototype.stop = function() {
	this._stop = true;
	for (var nano of this._nanos)
		if (nano !== undefined) {
			nano.stop();
			nano.disconnect();
		}
	for (var pool of this._pools)
		if (pool !== undefined)
			pool.disconnect();
};

Miner.prototype.log = function(level) {
	var args = Array.prototype.slice.call(arguments);
	args.shift();
	switch (level) {
		case "error":
			args[0] = "[MINER] " + arguments[1];
			console.error.apply(console, args);
			break;
		case "warn":
			args[0] = "[MINER] " + arguments[1];
			console.warn.apply(console, args);
			break;
		case "info":
			args[0] = "[MINER] " + arguments[1];
			console.info.apply(console, args);
			break;
		case "log1":
			args.unshift("%c[MINER] " + arguments[1]);
			args[1] = MINER_LOG1_STYLE;
			console.log.apply(console, args);
			break;
		case "log2":
			args.unshift("%c[MINER] " + arguments[1]);
			args[1] = MINER_LOG2_STYLE;
			console.log.apply(console, args);
			break;
		case "debug":
			args.unshift("%c[MINER] " + arguments[1]);
			args[1] = MINER_DEBUG_STYLE;
			console.debug.apply(console, args);
			break;
		default:
			break;
	}
};

var MinerEvent = function() {
	this._registered = [];
};

MinerEvent.prototype.addListener = function(callback) {
	return this._registered.push(callback) - 1;
};

MinerEvent.prototype.removeListener = function(id) {
	delete(this._registered[id]);
};

MinerEvent.prototype.fire = function(msg) {
	for (var callback of this._registered)
		if (callback !== undefined)
			callback(msg);
};
