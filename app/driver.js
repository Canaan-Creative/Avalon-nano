var Nano = function(device, miner) {
	this.device = device;
	this.miner = miner;
	this._sendCache = 0;
	this._enable = false;
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


Nano.prototype.run = function(size) {
	this._enable = true;
	var nano = this;

	var polling = mm_encode(
		P_POLLING, 0, 0x01, 0x01,
		new ArrayBuffer(32)
	);

	var i = 0;
	(function loop() {
		var work = nano.miner.getWork;
		while (work !== undefined) {
			for (var j in work)
				nano._send(work[j], j);
			nano._send(polling);
			nano._receive();
			i++;
			if (size === i)
				break;
			else
				work = nano.miner.getWork;
		}
		if (i < size)
			setTimeout(loop, 100);
	})();
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
		nano.log("debug", "Send:    0x%s", ab2hex(pkg));
		if (index === 1)
			nano.sent = index;
	});
};

Nano.prototype._receive = function() {
	var nano = this;
	chrome.hid.receive(this.connection.connectionId, function(reportId, pkg) {
		if (chrome.runtime.lastError) {
			nano.log("error", chrome.runtime.lastError.message);
			return;
		}
		nano.log("debug", "Receive: 0x%s", ab2hex(pkg));
		nano.received = pkg;
	});
};

Nano.prototype.__defineSetter__("received", function(pkg) {
	var data = mm_decode(pkg);
	switch (data.type) {
		case P_NONCE:
			this.log("log2", "Nonce:   0x%s", data.nonce.toString(16));
			this.miner.newNonce = {
				nanoId: this.id,
				nonce: data
			};
			break;
		case P_STATUS:
			this.log("log2", "Status:  %d MHz", data.frequency);
			this.miner.newStatus = {
				nanoId: this.id,
				frequency: data.frequency
			};
			break;
		case P_ACKDETECT:
			this.log("log1", "Version: %s", data.version);
			this.version = data.version;
			if (check_version(this.version))
				this.miner.nanoDetected = {
					nanoId: this.id,
					success: true
				};
			else {
				this.log("info", "Wrong Version.");
				this.miner.nanoDetected = {
					nanoId: this.id,
					success: false
				};
			}
			break;
	}
	if (this._enable) {
		this._send(mm_encode(
			P_POLLING, 0, 0x01, 0x01,
			new ArrayBuffer(32)
		));
		this._receive();
	}
});

Nano.prototype.__defineSetter__("sent", function(info) {
	if (this._enable) {
		var nano = this;
		(function loop() {
			var work = nano.miner.getWork;
			if (work !== undefined)
				for (var i in work)
					nano._send(work[i], i);
			else
				setTimeout(loop, 100);
		})();
	}
});

Nano.prototype.log = function(level) {
	var args = Array.prototype.slice.call(arguments);
	switch (level) {
		case "error":
			args[0] = "[NANO %d] " + arguments[1];
			args[1] = this.id;
			console.error.apply(console, args);
			break;
		case "warn":
			args[0] = "[NANO %d] " + arguments[1];
			args[1] = this.id;
			console.warn.apply(console, args);
			break;
		case "info":
			args[0] = "[NANO %d] " + arguments[1];
			args[1] = this.id;
			console.info.apply(console, args);
			break;
		case "log1":
			args.unshift("%c[NANO %d] " + arguments[1]);
			args[1] = NANO_LOG1_STYLE;
			args[2] = this.id;
			console.log.apply(console, args);
			break;
		case "log2":
			args.unshift("%c[NANO %d] " + arguments[1]);
			args[1] = NANO_LOG2_STYLE;
			args[2] = this.id;
			console.log.apply(console, args);
			break;
		case "debug":
			args.unshift("%c[NANO %d] " + arguments[1]);
			args[1] = NANO_DEBUG_STYLE;
			args[2] = this.id;
			console.debug.apply(console, args);
			break;
		default:
			break;
	}
};
