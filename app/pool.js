// TODO: error handling
Pool = function(poolInfo) {
	this.id = poolInfo.id;
	this.url = poolInfo.url;
	this.port = poolInfo.port;
	this.username = poolInfo.username;
	this.password = poolInfo.password;
	this.difficulty = 1;
	this.miner = miner;
	this.submitQueue = [];
	this.submitId = 0;

	this._SUBSCRIBE = {
		id: 1,
		method: "mining.subscribe",
		params: ["avalon-miner-chrome"]
	};
	this._AUTHORIZE = {
		id: 2,
		method: "mining.authorize",
		params: [this.username, this.password],
	};
};

Pool.prototype.run = function() {
	var pool = this;

	chrome.sockets.tcp.create({}, function(createInfo) {
		pool.socketId = createInfo.socketId;
		chrome.sockets.tcp.connect(pool.socketId, pool.url, pool.port, function(result) {
			if (chrome.runtime.lastError) {
				// TODO: alert somebody !!!
				//       a setter should work: pool.error = message
				pool.log("error", chrome.runtime.lastError.message);
				return;
			}
			pool.log("info", "Connected.");
			pool.upload(pool._SUBSCRIBE);
		});
	});
};

Pool.prototype.disconnect = function() {
	var pool = this;
	if (this.socketId !== undefined)
		chrome.sockets.tcp.disconnect(this.socketId, function() {
			chrome.sockets.tcp.close(pool.socketId, function(){
				pool.log("info", "Disconnected.");
			});
		});
};

Pool.prototype.decode = function(result) {
	var data = JSON.parse(result);
	switch (data.method) {
		case "mining.set_difficulty":
			this.difficulty = data.params[data.params.length - 1];
			break;
		case "mining.notify":
			this.miner.newJob = {
				poolId: this.id,
				nonce1: this.nonce1,
				nonce2_size: this.nonce2_size,
				job_id: data.params[0],
				prevhash: data.params[1],
				coinbase1: data.params[2],
				coinbase2: data.params[3],
				merkle_branch: data.params[4],
				version: data.params[5],
				nbits: data.params[6],
				ntime: data.params[7],
				clean_jobs: data.params[8],
				target: getTarget(this.difficulty)
			};
			break;
		case "mining.ping":
			this.upload({"id": data.id, "result": "pong", "error": null});
			break;
		case "client.reconnect":
			this.disconnect();
			this.run();
			break;
		default:
			if (data.id === 1) {
				if (data.error) {
					this.miner.poolSubscribed = {poolId: this.id, success: false};
					this.log("warn", "Subscription Failed.");
					return false;
				}
				this.miner.poolSubscribed = {poolId: this.id, success: true};
				this.log("log1", "Subscribed.");
				this.nonce1 = data.result[data.result.length - 2];
				this.nonce2_size = data.result[data.result.length - 1];
				if (data.result[0][0][0] === 'mining.set_difficulty')
					this.difficulty = data.result[0][0][1];
				this.upload(this._AUTHORIZE);
			} else if (data.id === 2) {
				if (data.error) {
					this.miner.poolAuthorized = {poolId: this.id, success: false};
					this.log("warn", "Authorization Failed.");
					return false;
				}
				this.miner.poolAuthorized = {poolId: this.id, success: true};
				this.log("log1", "Authorized.");
			} else if (data.id >= 1000){
				if (data.error) {
					this.log("warn", "Submission Failed.");
					return false;
				}
				if (!data.result) {
					this.log("log2", "Submission Failed: %s.",
						data["reject-reason"]);
					return false;
				}
				this.log("log1", "Submitted.");
			}
			break;
	}
};

Pool.prototype.submit = function(jobId, nonce2, ntime, nonce) {
	var data = {
		params: [this.username, jobId, nonce2, ntime, nonce],
		id: 1000 + this.submitId,
		method: "mining.submit",
	};
	this.submitId = (this.submitId + 1) % 1000;
	this.submitQueue[this.submitId] = {
		data: data,
		retry: 0,
	};
	this.upload(data);
};

Pool.prototype.upload = function(data) {
	var pool = this;
	var dataStr = JSON.stringify(data);
	this.log("debug", "Sent:     %s", dataStr);
	dataStr = str2ab(dataStr + "\n");
	chrome.sockets.tcp.send(this.socketId, dataStr, function(sendInfo) {
		if (chrome.runtime.lastError) {
			// TODO: net::ERR_SOCKET_NOT_CONNECTED
			//       alert somebody !!!
			pool.log("error", chrome.runtime.lastError.message);
			if (data.method === "mining.submit") {
				var submitId = data.id - 1000;
				if (++(pool.submitQueue[submitId].retry) < 3)
					setTimeout(function() {
						pool.upload(pool.submitQueue[submitId].data);
					}, 1000);
				else
					delete(pool.submitQueue[submitId]);
			}
			return;
		}
		if (data.method === "mining.submit")
			delete(pool.submitQueue[data.id - 1000]);
	});
};

Pool.prototype.download = function(info) {
	var results = ab2str(info.data).split("\n");
	for (var i = 0; i < results.length - 1; i++) {
		this.log("debug", "Received: %s", results[i]);
		this.decode(results[i]);
	}
};

Pool.prototype.log = function(level) {
	var args = Array.prototype.slice.call(arguments);
	switch (level) {
		case "error":
			if (POOL_LOG_LIMIT > 4)
				break;
			args[0] = "[POOL %d] " + arguments[1];
			args[1] = this.id;
			console.error.apply(console, args);
			break;
		case "warn":
			if (POOL_LOG_LIMIT > 3)
				break;
			args[0] = "[POOL %d] " + arguments[1];
			args[1] = this.id;
			console.warn.apply(console, args);
			break;
		case "info":
			if (POOL_LOG_LIMIT > 2)
				break;
			args[0] = "[POOL %d] " + arguments[1];
			args[1] = this.id;
			console.info.apply(console, args);
			break;
		case "log1":
			if (POOL_LOG_LIMIT > 1)
				break;
			args.unshift("%c[POOL %d] " + arguments[1]);
			args[1] = POOL_LOG1_STYLE;
			args[2] = this.id;
			console.log.apply(console, args);
			break;
		case "log2":
			if (POOL_LOG_LIMIT > 1)
				break;
			args.unshift("%c[POOL %d] " + arguments[1]);
			args[1] = POOL_LOG2_STYLE;
			args[2] = this.id;
			console.log.apply(console, args);
			break;
		case "debug":
			if (POOL_LOG_LIMIT > 0)
				break;
			args.unshift("%c[POOL %d] " + arguments[1]);
			args[1] = POOL_DEBUG_STYLE;
			args[2] = this.id;
			console.debug.apply(console, args);
			break;
		default:
			break;
	}
};
