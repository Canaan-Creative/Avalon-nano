Pool = function(url, port, username, password) {
	this.url = url;
	this.port = port;
	this.username = username;
	this.password = password;
};

Pool.prototype.init = function() {
	var pool = this;

	var SUBSCRIBE = {
		id: 1,
		method: "mining.subscribe",
		params: ["avalon-app"]
	};
	var AUTHORIZE = {
		id: 2,
		method: "mining.authorize",
		params: [this.username, this.password]
	}

	var download = function(info) {
		if (info.socketId != pool.socketId)
			return;
		var result = ab2str(info.data);
		console.log(result);
		result = JSON.parse(result);
		switch (result.id) {
			case 1:
				pool.upload(AUTHORIZE);
				break;
			default:
				//console.log(result);
				break;
		}
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

Pool.prototype.upload = function(data) {
	console.log(data);
	data = str2ab(JSON.stringify(data) + "\n");
	chrome.sockets.tcp.send(this.socketId, data, function(sendInfo) {
		//console.log(sendInfo);
	});
};
