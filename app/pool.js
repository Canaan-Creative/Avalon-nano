Pool = function(url, port, username, password) {
	this.url = url;
	this.port = port;
	this.username = username;
	this.password = password;
};

Pool.prototype.run = function() {
	var pool = this;

	var SUBSCRIBE = {
		id: 1,
		method: "mining.subscribe",
		params: ["avalon-app"]
	};

	var download = function(info) {
		if (info.socketId != pool.socketId)
			return;
		var results = ab2str(info.data).split("\n");

		for (var i = 0; i < results.length - 1; i++)
			pool.decode(results[i]);
	};
	chrome.sockets.tcp.create({}, function(createInfo) {
		pool.socketId = createInfo.socketId;
		chrome.sockets.tcp.connect(pool.socketId, pool.url, pool.port, function(result) {
			chrome.sockets.tcp.onReceive.addListener(download);
			pool.upload(SUBSCRIBE);
		});
	});
};

Pool.prototype.disconnect = function() {
};

Pool.prototype.decode = function(result) {
	var AUTHORIZE = {
		id: 2,
		method: "mining.authorize",
		params: [this.username, this.password]
	};
	var data = JSON.parse(result);
	switch (data.id) {
		case 1:
			console.log(data);
			pool.upload(AUTHORIZE);
			break;
		default:
			// TODO: get work from this DATA
			console.log(data);
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
