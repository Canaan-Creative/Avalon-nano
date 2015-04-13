var Nano = function(device, connection) {
	this.device = device;
	this.connection = connection;
	this.in_buffer = [];
	this.out_buffer = [];
	this.BUFFER_SIZE = 32;
};

Nano.prototype.push_data = function(data) {
	var pkgs = [];
	var cnt = Math.ceil(data.byteLength / 33);
	for (var idx = 1; idx < cnt + 1; idx++) {
		pkgs.push(mm_encode(
			P_WORK, idx, cnt, data.slice((idx - 1) * 32, idx * 32)
		));
	}
	this.out_buffer.push(pkgs);
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

Nano.prototype.work = function(pkgs, ntime, callback) {
	var nano = this;
	for (var pkg of pkgs)
		this._send(pkg);

	var receive_callback = function(pkg) {
		nano.in_buffer.push(pkg);
		callback();

		// TODO: what if it is not a P_NONCE package?
		//if (data.type === P_NONCE) {
		//	console.log("%cNonce:   0x%s", LOG2_STYLE, data.nonce.toString(16));
		//}
	};

	for (var i = 0; i < ntime; i++)
		this._receive(receive_callback);
};

Nano.prototype.run = function(prepkgs) {
	var nano = this;
	var callback = function(pkg) {
		nano.in_buffer.push(pkg);
	};
	var loop = function() {
		if (nano.stop) {
			console.info("Stopped.");
			for (var j = 1; j < prepkgs; j++) {
				nano._receive(callback);
			}
			return;
		}
		var pkgs = nano.out_buffer.shift();

		// TODO: should wait until pkgs is not null
		nano.work(pkgs, 1, loop);
	};

	this.detect(function() {
		if (check_version(nano.version)) {
			nano.stop = false;

			// send some packages to fill the buffer of nano
			for (var i = 1; i < prepkgs; i++) {
				var pkgs = nano.out_buffer.shift();
				for (var pkg of pkgs)
					nano._send(pkg);
			}
			loop();
		}
	});

	var decode = setInterval(function() {
		var pkg = nano.in_buffer.shift();
		if (nano.stop && pkg === undefined)
			clearInterval(decode);
		else if (pkg !== undefined) {
			var data = mm_decode(pkg);

			// TODO: what if it is not a P_NONCE package?
			if (data.type === P_NONCE)
				console.log("%cNonce:   0x%s", LOG2_STYLE, data.nonce.toString(16));
		}
	}, 10);
};

Nano.prototype._send = function(pkg) {
	chrome.hid.send(this.connection.connectionId, 0, pkg, function() {
		if (chrome.runtime.lastError) {
			console.error(chrome.runtime.lastError);
			return;
		}
		console.debug("%cSend:    0x%s", DEBUG_STYLE, ab2str(pkg));
	});
};

Nano.prototype._receive = function(callback) {
	chrome.hid.receive(this.connection.connectionId, function(reportId, pkg) {
		if (chrome.runtime.lastError) {
			console.error(chrome.runtime.lastError);
			return;
		}
		console.debug("%cReceive: 0x%s", DEBUG_STYLE, ab2str(pkg));
		callback(pkg);
	});
};
