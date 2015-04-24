// TODO: logging
// TODO: error handling
Pool = function(poolInfo) {
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

		for (var i = 0; i < results.length - 1; i++)
			pool.decode(results[i]);
	};
	chrome.sockets.tcp.create({}, function(createInfo) {
		pool.socketId = createInfo.socketId;
		chrome.sockets.tcp.connect(pool.socketId, pool.url, pool.port, function(result) {
			chrome.sockets.tcp.onReceive.addListener(download);
			pool.upload(pool._SUBSCRIBE);
		});
	});
};

Pool.prototype.disconnect = function() {
};

Pool.prototype.decode = function(result) {
	// TODO: what if data.error !== null

	var data = JSON.parse(result);
	if (data.id === 1) {
		console.log(data);
		this.nonce1 = data.result[data.result.length - 2];
		this.nonce2_size = data.result[data.result.length - 1];
		if (data.result[0][0][0] === 'mining.set_difficulty')
			this.difficulty = data.result[0][0][1];
		this.upload(this._AUTHORIZE);
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
	console.log(data);
	data = str2ab(JSON.stringify(data) + "\n");
	chrome.sockets.tcp.send(this.socketId, data, function(sendInfo) {
		//console.log(sendInfo);
	});
};
