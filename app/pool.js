// TODO: error handling
var Pool = function(id, url, port, username, password) {
	var difficulty = 1;
	var nonce1 = null;
	var nonce2Size = null;

	var submitId = 0;
	var AUTHORIZE = Pool.stratumEncode({
		id: 2,
		method: "mining.authorize",
		params: [username, password || "1234"],
	});

	var header = "POOL" + id;
	var watcherId;
	var pool = this;

	this.socketId = null;
	this.id = id;
	this.alive = true;
	this.onJob = new MinerEvent();
	this.onError = new MinerEvent();

	var watcher = function() {
		chrome.sockets.tcp.create({}, function(createInfo) {
			var socketId = createInfo.socketId;
			var error = false;
			var wait = setTimeout(function() {
				error = true;
				utils.log("warn", ["Connection lost (Timed out)"], header, "color: red");
				pool.alive = false;
				pool.onError.fire(pool.id);
			}, 5000);
			chrome.sockets.tcp.connect(socketId, url, port, function(result) {
				clearTimeout(wait);
				if (chrome.runtime.lastError) {
					if (error) {
						chrome.sockets.tcp.close(socketId, function() {});
						return;
					}
					utils.log("warn", ["Connection lost"], header, "color: red");
					pool.alive = false;
					pool.onError.fire(pool.id);
					chrome.sockets.tcp.close(socketId, function() {});
					return;
				}
				chrome.sockets.tcp.disconnect(socketId, function() {
					chrome.sockets.tcp.close(socketId, function() {
						clearTimeout(watcherId);
						watcherId = setTimeout(watcher, 5000);
					});
				});
			});
		});
	};

	var send = function(data, retry) {
		utils.log("log", ["Sent:     %s", utils.ab2asc(data)],
			header, "color: darksalmon");
		chrome.sockets.tcp.send(pool.socketId, data, function(sendInfo) {
			if (chrome.runtime.lastError) {
				utils.log("error", [chrome.runtime.lastError.message], header);
				if (retry)
					send(data, retry - 1);
				return;
			}
		});
	};

	this.receive = function(stratum) {
		clearTimeout(watcherId);
		watcherId = setTimeout(watcher, 1000);
		utils.log("log", ["Received: %s", utils.ab2asc(stratum)],
			header, "color: goldenrod");
		for (var data of Pool.stratumDecode(stratum)) {
			decode(data);
		}
	};

	var decode = function(data) {
		switch (data.method) {
		case "mining.set_difficulty":
			difficulty = data.params[data.params.length - 1];
			break;
		case "mining.notify":
			var job = {
				poolId: pool.id,
				nonce1: nonce1,
				nonce2Size: nonce2Size,
				jobId: data.params[0],
				prevhash: data.params[1],
				coinbase1: data.params[2],
				coinbase2: data.params[3],
				merkleBranch: data.params[4],
				version: data.params[5],
				nbits: data.params[6],
				ntime: data.params[7],
				cleanJobs: data.params[8],
				target: utils.getTarget(difficulty)
			};
			pool.onJob.fire(job);
			break;
		case "mining.ping":
			send(Pool.stratumEncode({
				"id": data.id,
				"result": "pong",
				"error": null
			}));
			break;
		case "client.reconnect":
			pool.disconnect();
			pool.connect();
			break;
		default:
			if (data.id === 1) {
				if (data.error) {
					// this.log("warn", "Subscription Failed.");
					return false;
				}
				// this.log("log1", "Subscribed.");
				nonce1 = data.result[data.result.length - 2];
				nonce2Size = data.result[data.result.length - 1];
				if (data.result[0][0][0] === 'mining.set_difficulty')
					difficulty = data.result[0][0][1];
				send(AUTHORIZE);
			} else if (data.id === 2) {
				if (data.error) {
					// this.log("warn", "Authorization Failed.");
					return false;
				}
				// this.log("log1", "Authorized.");
			} else if (data.id >= 1000){
				if (data.error) {
					// this.log("warn", "Submission Failed.");
					return false;
				}
				if (!data.result) {
					// this.log("log2", "Submission Failed: %s.",
					//	data["reject-reason"]);
					return false;
				}
			}
			break;
		}
	};

	this.connect = function() {
		chrome.sockets.tcp.create({}, function(createInfo) {
			pool.socketId = createInfo.socketId;
			var error = false;
			var wait = setTimeout(function() {
				error = true;
				utils.log("warn", ["Connection failed (Timed out)"], header, "color: red");
				pool.alive = false;
				pool.onError.fire(pool.id);
			}, 5000);
			chrome.sockets.tcp.connect(pool.socketId, url, port, function(result) {
				clearTimeout(wait);
				if (chrome.runtime.lastError) {
					if (error) {
						chrome.sockets.tcp.close(pool.socketId, function() {});
						return;
					}
					utils.log("warn", ["Connection failed"], header, "color: red");
					pool.alive = false;
					pool.onError.fire(pool.id);
					chrome.sockets.tcp.close(pool.socketId, function() {});
					return;
				}
				utils.log("info", ["Connected"], header, "color: maroon");
				pool.alive = true;
				send(Pool.SUBSCRIBE);
				watcherId = setTimeout(watcher, 5000);
			});
		});
	};

	this.disconnect = function() {
		clearTimeout(watcherId);
		if (this.socketId !== null)
			chrome.sockets.tcp.disconnect(this.socketId, function() {
				chrome.sockets.tcp.close(pool.socketId, function() {
					utils.log("info", ["Disconnected"], header, "color: maroon");
				});
			});
	};

	this.submit = function(jobId, nonce2, ntime, nonce) {
		utils.log("info", ["Submitted"], header, "color: orangered");
		var data = {
			params: [username, jobId, nonce2, ntime, nonce],
			id: 1000 + submitId,
			method: "mining.submit",
		};
		submitId = (submitId + 1) % 1000;
		send(Pool.stratumEncode(data), 3);
	};
};

Pool.stratumEncode = function(data) {
	return utils.str2ab(JSON.stringify(data) + "\n");
};

Pool.stratumDecode = function(stratum) {
	return utils.ab2asc(stratum).slice(0, -1).split("\n").map(function(data) {
		return JSON.parse(data);
	});
};

Pool.SUBSCRIBE = Pool.stratumEncode({
	id: 1,
	method: "mining.subscribe",
	params: ["avalon-miner-chrome"]
});
