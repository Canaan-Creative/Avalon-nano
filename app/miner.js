var Miner = function() {
	// Events
	this.onNewNano = new MinerEvent();
	this.onNanoDeleted = new MinerEvent();
	this.onNanoConnected = new MinerEvent();
	this.onNanoDetected = new MinerEvent();
	this.onNewNonce = new MinerEvent();
	this.onNewStatus = new MinerEvent();
	this.onPoolSubscribed = new MinerEvent();
	this.onPoolAuthorized = new MinerEvent();

	this._JOB_BUFFER_SIZE = 256;
	this._WORK_BUFFER_SIZE = 1024;

	this._jobId = [];
	this._poolId = 0;

	this._jobs = [];
	this._works = [];

	this._nanos = [];
	this._pools = [];

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
	msg.nano.connect();

	this.onNewNano.fire(msg);
});

Miner.prototype.__defineSetter__("nanoDeleted", function(nanoId) {
	this.log("info", "Nano Deleted: %d", nanoId);
	this._nanos[nanoId].stop();
	delete(this._nanos[nanoId]);
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
	// msg: {nanoId, nonce}
	// TODO: submit
	this.onNewNonce.fire(msg);
});

Miner.prototype.__defineSetter__("newStatus", function(msg) {
	// msg: {nanoId, stat}
	// TODO: update status
	this.onNewStatus.fire(msg);
});

Miner.prototype.__defineGetter__("getWork", function() {
	if (this._active_pool === undefined)
		return undefined;
	var work = this._works[this._active_pool].shift();
	if (this._thread_pause[this._active_pool]
			&& (this._works[this._active_pool].length < this._WORK_BUFFER_SIZE - 10)) {
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
