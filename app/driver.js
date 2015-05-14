var Nano = function(device, miner) {
	this.device = device;
	this.miner = miner;
	this._enable = false;
	this.frequency = null;
};

Nano.prototype.__defineGetter__("id", function() {
	return this.device.deviceId;
});

Nano.prototype.connect = function() {
	var nano = this;
	chrome.hid.connect(this.id, function(connection) {
		if (chrome.runtime.lastError) {
			nano.log("error", chrome.runtime.lastError.message);
			nano.miner.nanoConnected = {
				nanoId: nano.id,
				success: false
			};
			return;
		}
		nano.connection = connection;
		nano.miner.nanoConnected = {
			nanoId: nano.id, success: true
		};
		nano._receive();
	});
};

Nano.prototype.disconnect = function() {
	var nano = this;
	chrome.hid.disconnect(nano.connection.connectionId, function() {
		nano.log("info", "Disconnected.");
	});
};


Nano.prototype.run = function() {
	this._enable = true;
	var size = this.asicCount || 1;
	var nano = this;

	var polling = mm_encode(
		P_POLLING, 0, 0x01, 0x01,
		new ArrayBuffer(32)
	);
	var set_volt = mm_encode(
		P_SET_VOLT, 0, 1, 1,
		hex2ab("009f")
	);
	nano._send(set_volt);

	var i = 0;
	var loop = function() {
		var work = nano.miner.getWork;
		while (work !== undefined) {
			//nano._send(polling);
			//nano._receive();
			for (var j = 0; j < work.length; j++)
				nano._send(work[j], j);
			i++;
			if (i === size)
				break;
			else
				work = nano.miner.getWork;
		}
		if (i < size)
			setTimeout(loop, 20);
		if (i === size) {
			i = 0;
			nano._send(polling);
			nano._receive();
			setTimeout(loop, 175);
		}
	};
	setTimeout(loop, 1000);
};

Nano.prototype.stop = function() {
	this._enable = false;
	this.log("info", "Stopped.");
};

Nano.prototype.detect = function() {
	this._send(mm_encode(
		P_DETECT, 0, 0x01, 0x01,
		new ArrayBuffer(32)
	));
	this._receive();
};

Nano.prototype._send = function(pkg, index) {
	var nano = this;
	chrome.hid.send(this.connection.connectionId, 0, pkg, function() {
		if (chrome.runtime.lastError) {
			nano.log("error", chrome.runtime.lastError.message);
			return;
		}
		nano.log("debug", "Sent:     0x%s", ab2hex(pkg));
	});
};

Nano.prototype._receive = function() {
	var nano = this;
	chrome.hid.receive(this.connection.connectionId, function(reportId, pkg) {
		if (chrome.runtime.lastError) {
			nano.log("error", chrome.runtime.lastError.message);
			return;
		}
		nano.log("debug", "Received: 0x%s", ab2hex(pkg));
		nano.received = pkg;
	});
};

Nano.prototype.__defineSetter__("received", function(pkg) {
	var data = mm_decode(pkg);
	switch (data.type) {
		case P_NONCE:
			for (var p of data.value) {
				this.log("log2", "Nonce:    0x%s", p.nonce.toString(16));
				this.miner.newNonce = {
					nanoId: this.id,
					nonce: p.nonce,
					nonce2: p.nonce2,
					jobId: p.jobId,
					poolId: p.poolId,
					ntime: p.ntime,
				};
			}
			break;
		case P_STATUS:
			this.log("log2", "Status:   %d MHz", data.frequency);
			if (this.frequency !== data.frequency) {
				this.frequency = data.frequency;
				this.miner.newStatus = {
					nanoId: this.id,
					stat: {frequency: data.frequency}
				};
			}
			break;
		case P_ACKDETECT:
			this.log("log1", "Version:  %s", data.version);
			this.log("log1", "DNA:      %s", data.dna);
			this.log("log1", "AsicCount:%s", data.asicCount);
			this.version = data.version;
			this.dna = data.dna;
			this.asicCount = data.asicCount;
			if (check_version(this.version))
				this.miner.nanoDetected = {
					nanoId: this.id,
					version: this.version,
					success: true,
				};
			else {
				this.log("info", "Wrong Version.");
				this.miner.nanoDetected = {
					nanoId: this.id,
					version: this.version,
					success: false
				};
			}
			break;
	}
});

Nano.prototype.__defineSetter__("sent", function(info) {
	if (this._enable) {
		var nano = this;
		(function loop() {
			var work = nano.miner.getWork;
			if (work !== undefined) {
				nano._send(mm_encode(
					P_POLLING, 0, 0x01, 0x01,
					new ArrayBuffer(32)
				));
				nano._receive();
				for (var i = 0; i < work.length; i++)
					nano._send(work[i], i);
			} else
				setTimeout(loop, 100);
		})();
	}
});

Nano.prototype.log = function(level) {
	var args = Array.prototype.slice.call(arguments);
	switch (level) {
		case "error":
			if (NANO_LOG_LIMIT > 4)
				break;
			args[0] = "[NANO %d] " + arguments[1];
			args[1] = this.id;
			console.error.apply(console, args);
			break;
		case "warn":
			if (NANO_LOG_LIMIT > 3)
				break;
			args[0] = "[NANO %d] " + arguments[1];
			args[1] = this.id;
			console.warn.apply(console, args);
			break;
		case "info":
			if (NANO_LOG_LIMIT > 2)
				break;
			args[0] = "[NANO %d] " + arguments[1];
			args[1] = this.id;
			console.info.apply(console, args);
			break;
		case "log1":
			if (NANO_LOG_LIMIT > 1)
				break;
			args.unshift("%c[NANO %d] " + arguments[1]);
			args[1] = NANO_LOG1_STYLE;
			args[2] = this.id;
			console.log.apply(console, args);
			break;
		case "log2":
			if (NANO_LOG_LIMIT > 1)
				break;
			args.unshift("%c[NANO %d] " + arguments[1]);
			args[1] = NANO_LOG2_STYLE;
			args[2] = this.id;
			console.log.apply(console, args);
			break;
		case "debug":
			if (NANO_LOG_LIMIT > 0)
				break;
			args.unshift("%c[NANO %d] " + arguments[1]);
			args[1] = NANO_DEBUG_STYLE;
			args[2] = this.id;
			console.debug.apply(console, args);
			break;
		default:
			break;
	}
};
