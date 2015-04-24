// TODO: logging
// TODO: error handling
Pool = function(poolInfo) {
	this.id = poolInfo.id;
	this.url = poolInfo.url;
	this.port = poolInfo.port;
	this.username = poolInfo.username;
	this.password = poolInfo.password;
	this.miner = miner;

	this._SUBSCRIBE = {
		id: 1,
		method: "mining.subscribe",
		params: ["avalon-nano-app"]
	};
	this._AUTHORIZE = {
		id: 2,
		method: "mining.authorize",
		params: [this.username, this.password],
	};
};

Pool.prototype.run = function() {
	var pool = this;

	var download = function(info) {
		if (info.socketId !== pool.socketId)
			return;
		var results = ab2str(info.data).split("\n");

		for (var i = 0; i < results.length - 1; i++) {
			pool.log("debug", "Received: %s", results[i]);
			pool.decode(results[i]);
		}
	};
	chrome.sockets.tcp.create({}, function(createInfo) {
		pool.socketId = createInfo.socketId;
		chrome.sockets.tcp.connect(pool.socketId, pool.url, pool.port, function(result) {
			pool.log("info", "Connected.");
			chrome.sockets.tcp.onReceive.addListener(download);
			pool.upload(pool._SUBSCRIBE);
		});
	});
};

Pool.prototype.disconnect = function() {
	chrome.sockets.tcp.disconnect(this.socketId);
	pool.log("info", "Disconnected.");
};

Pool.prototype.decode = function(result) {
	// TODO: what if data.error !== null

	var data = JSON.parse(result);
	if (data.id === 1) {
		if (data.error) {
			this.log("warn", "Subscription Failed.");
			return false;
		}
		this.log("log1", "Subscibed.");
		this.nonce1 = data.result[data.result.length - 2];
		this.nonce2_size = data.result[data.result.length - 1];
		if (data.result[0][0][0] === 'mining.set_difficulty')
			this.difficulty = data.result[0][0][1];
		this.upload(this._AUTHORIZE);
	} else if (data.id === 2) {
		if (data.error) {
			this.log("warn", "Authentication Failed.");
			return false;
		}
		this.log("log1", "Authenticated.");
	} else if (data.id === 3) {
		if (data.error) {
			this.log("warn", "Submission Failed.");
			return false;
		}
		this.log("log2", "Submitted.");
	} else switch (data.method) {
		case "mining.set_difficulty":
			this.difficulty = data.params[0];
			break;
		case "mining.notify":
			this.miner.newJob = {
				nonce1: this.nonce1,
				job_id: data.params[0],
				prevhash: data.params[1],
				coinbase1: data.params[2],
				coinbase2: data.params[3],
				merkle_branch: data.params[4],
				version: data.params[5],
				nbits: data.params[6],
				ntime: data.params[7],
				clean_jobs: data.params[8]
			};
			break;
	}
};

Pool.prototype.upload = function(data) {
	data = JSON.stringify(data);
	this.log("debug", "Sent:     %s", data);
	data = str2ab(data + "\n");
	chrome.sockets.tcp.send(this.socketId, data, function(sendInfo) {
		//console.log(sendInfo);
	});
};

Pool.prototype.log = function(level) {
	var args = Array.prototype.slice.call(arguments);
	switch (level) {
		case "error":
			args[0] = "[POOL %d] " + arguments[1];
			args[1] = this.id;
			console.error.apply(console, args);
			break;
		case "warn":
			args[0] = "[POOL %d] " + arguments[1];
			args[1] = this.id;
			console.warn.apply(console, args);
			break;
		case "info":
			args[0] = "[POOL %d] " + arguments[1];
			args[1] = this.id;
			console.info.apply(console, args);
			break;
		case "log1":
			args.unshift("%c[POOL %d] " + arguments[1]);
			args[1] = POOL_LOG1_STYLE;
			args[2] = this.id;
			console.log.apply(console, args);
			break;
		case "log2":
			args.unshift("%c[POOL %d] " + arguments[1]);
			args[1] = POOL_LOG2_STYLE;
			args[2] = this.id;
			console.log.apply(console, args);
			break;
		case "debug":
			args.unshift("%c[POOL %d] " + arguments[1]);
			args[1] = POOL_DEBUG_STYLE;
			args[2] = this.id;
			console.debug.apply(console, args);
			break;
		default:
			break;
	}
};
