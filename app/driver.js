var Nano = function(device) {
	this.device = device;
	this._in_buffer = [];
	this._out_buffer = [];
	this._send_queue = 0;
	this._BUFFER_SIZE = 32;
};

Nano.prototype.push_data = function(data) {
	if (this._out_buffer.length >= this._BUFFER_SIZE)
		return false;
	var pkgs = [];
	var cnt = Math.ceil(data.byteLength / 33);
	for (var idx = 1; idx < cnt + 1; idx++) {
		pkgs.push(mm_encode(
			P_WORK, idx, cnt, data.slice((idx - 1) * 32, idx * 32)
		));
	}
	this._out_buffer.push(pkgs);
	return true;
};

Nano.prototype.detect = function(callback) {
	var nano = this;
	this._send(mm_encode(
		P_DETECT, 0x01, 0x01,
		new ArrayBuffer(32)
	));
	this._receive(function(pkg) {
		var data = mm_decode(pkg);

		// TODO: what if it is not a P_ACKDETECT package?
		if (data.type === P_ACKDETECT) {
			nano.log("log1", "Version: %s", data.version);
			nano.version = data.version;
			callback();
		}
	});
};

Nano.prototype.run = function(queue_size) {
	var nano = this;
	this.detect(function() {
		if (check_version(nano.version)) {
			nano._stop = false;
			nano._receive_loop();
			nano._decode_loop();
			nano._send_loop(queue_size);
		}
	});
};

Nano.prototype.stop = function() {
	this._stop = true;
	this.log("info", "Stopped.");
};

Nano.prototype.connect = function() {
	var nano = this;
	chrome.hid.connect(this.device.deviceId, function(connection) {
		if (chrome.runtime.lastError) {
			nano.log("error", chrome.runtime.lastError.message);
			return;
		}
		nano.connection = connection;
	});
};

Nano.prototype.disconnect = function() {
	var nano = this;
	chrome.hid.disconnect(nano.connection.connectionId, function() {
		nano.log("info", "Disconnected.");
	});
};

Nano.prototype.log = function(level) {
	var args = Array.prototype.slice.call(arguments);
	var i;
	switch (level) {
		case 'error':
			args[0] = "[NANO %d] " + arguments[1];
			args[1] = this.device.deviceId;
			console.error.apply(console, args);
			break;
		case 'warn':
			args[0] = "[NANO %d] " + arguments[1];
			args[1] = this.device.deviceId;
			console.warn.apply(console, args);
			break;
		case 'info':
			args[0] = "[NANO %d] " + arguments[1];
			args[1] = this.device.deviceId;
			console.info.apply(console, args);
			break;
		case 'log1':
			args.unshift("%c[NANO %d] " + arguments[1]);
			args[1] = LOG1_STYLE;
			args[2] = this.device.deviceId;
			console.log.apply(console, args);
			break;
		case 'log2':
			args.unshift("%c[NANO %d] " + arguments[1]);
			args[1] = LOG2_STYLE;
			args[2] = this.device.deviceId;
			console.log.apply(console, args);
			break;
		case 'debug':
			args.unshift("%c[NANO %d] " + arguments[1]);
			args[1] = DEBUG_STYLE;
			args[2] = this.device.deviceId;
			console.debug.apply(console, args);
			break;
		default:
			break;
	}
};

Nano.prototype._decode_loop = function() {
	var nano = this;
	var decode = setInterval(function() {
		var pkg = nano._in_buffer.shift();
		if (nano._stop && pkg === undefined)
			clearInterval(decode);
		else if (pkg !== undefined) {
			var data = mm_decode(pkg);

			// TODO: what if it is not a P_NONCE package?
			if (data.type === P_NONCE)
				nano.log("log2", "Nonce:   0x%s", data.nonce.toString(16));
		}
	}, 50);
};

Nano.prototype._send_loop = function(queue_size) {
	var nano = this;
	var loop = setInterval(function() {
		if (nano._stop)
			clearInterval(loop);
		else
			while (nano._send_queue < queue_size) {
				var pkgs = nano._out_buffer.shift();
				if (pkgs === undefined)
					break;
				for (var pkg of pkgs)
					nano._send(pkg);
			}
	}, 50);
};

Nano.prototype._receive_loop = function() {
	var nano = this;
	var callback = function(pkg) {
		nano._in_buffer.push(pkg);
		nano._receive(callback);
	};
	this._receive(callback);
};

Nano.prototype._send = function(pkg) {
	var nano = this;
	this._send_queue += 1;
	chrome.hid.send(this.connection.connectionId, 0, pkg, function() {
		if (chrome.runtime.lastError) {
			nano.log("error", chrome.runtime.lastError.message);
			return;
		}
		nano.log("debug", "Send:    0x%s", ab2str(pkg));
		nano._send_queue -= 1;
	});
};

Nano.prototype._receive = function(callback) {
	var nano = this;
	chrome.hid.receive(this.connection.connectionId, function(reportId, pkg) {
		if (chrome.runtime.lastError) {
			nano.log("error", chrome.runtime.lastError.message);
			return;
		}
		nano.log("debug", "Receive: 0x%s", ab2str(pkg));
		callback(pkg);
	});
};
