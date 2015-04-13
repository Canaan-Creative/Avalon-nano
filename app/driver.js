var Nano = function(device, connection) {
	this.device = device;
	this.connection = connection;
	this._in_buffer = [];
	this._out_buffer = [];
	this._send_queue = 0;
	this.BUFFER_SIZE = 32;
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
			console.log("%cVersion: %s", LOG1_STYLE, data.version);
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
			nano._send_loop(5);
		}
	});
};

Nano.prototype.stop = function() {
    this._stop = true;
    console.info("Stopped");
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
				console.log("%cNonce:   0x%s", LOG2_STYLE, data.nonce.toString(16));
		}
	}, 10);
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
	}, 10);
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
			console.error(chrome.runtime.lastError.message);
			return;
		}
		console.debug("%cSend:    0x%s", DEBUG_STYLE, ab2str(pkg));
		nano._send_queue -= 1;
	});
};

Nano.prototype._receive = function(callback) {
	chrome.hid.receive(this.connection.connectionId, function(reportId, pkg) {
		if (chrome.runtime.lastError) {
			console.error(chrome.runtime.lastError.message);
			return;
		}
		console.debug("%cReceive: 0x%s", DEBUG_STYLE, ab2str(pkg));
		callback(pkg);
	});
};
