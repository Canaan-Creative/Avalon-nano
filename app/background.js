var setting = [];
var miner;

var main = function() {
	miner = new Miner();
	miner.onNewNano.addListener(function(msg) {
		chrome.runtime.sendMessage({
			info: "NewNano",
			nanoId: msg.nanoId,
			nano: msg.nano
		});
	});

	miner.onNanoDeleted.addListener(function(nanoId) {
		chrome.runtime.sendMessage({
			info: "NanoDeleted",
			nanoId: nanoId
		});
	});

	miner.onNanoConnected.addListener(function(msg) {
		chrome.runtime.sendMessage({
			info: "NanoConnected",
			nanoId: msg.nanoId,
			succes: msg.success
		});
	});

	miner.onNanoDetected.addListener(function(msg) {
		chrome.runtime.sendMessage({
			info: "NanoDetected",
			nanoId: msg.nanoId,
			succes: msg.success
		});
	});

	miner.onNewNonce.addListener(function(msg) {
		chrome.runtime.sendMessage({
			info: "NewNonce",
			nanoId: msg.nanoId,
			nonce: msg.nonce
		});
	});

	miner.onNewStatus.addListener(function(msg) {
		chrome.runtime.sendMessage({
			info: "NewStatus",
			nanoId: msg.nanoId,
			stat: msg.stat
		});
	});

	miner.onPoolSubscribed.addListener(function(msg) {
		chrome.runtime.sendMessage({
			info: "PoolSubscribed",
			poolId: msg.poolId,
			success: msg.success
		});
	});

	miner.onPoolAuthorized.addListener(function(msg) {
		chrome.runtime.sendMessage({
			info: "PoolAuthorized",
			poolId: msg.poolId,
			success: msg.success
		});
	});

	for (var i = 0; i < setting.length; i++)
		if (setting[i] !== undefined)
			miner.setPool({
				url: setting[i].url,
				port: setting[i].port,
				username: setting[i].username,
				password: setting[i].password
			}, i);
};


chrome.app.runtime.onLaunched.addListener(function() {
	chrome.storage.local.get("pool", function(result) {
		setting = result.pool;
		setting = [{
			url: "stratum.btcchina.com",
			port: 3333,
			username: "canaan.apptest",
			password: "1234"
		}];
	});

	chrome.runtime.onMessage.addListener(function(msg, sender) {
		if (sender.url.indexOf("index.html") > -1)
			switch (msg.info) {
				case "Ready":
					if (setting !== undefined)
					chrome.runtime.sendMessage({
						info: "PoolInit",
						setting: setting
					});
					main();
					break;
				case "NewPool":
					miner.setPool({
						url: msg.data.url,
						port: msg.data.port,
						username: msg.data.username,
						password: msg.data.password || "1234"
					}, msg.data.poolId);
					setting[msg.data.poolId] = {
						url: msg.data.url,
						port: msg.data.port,
						username: msg.data.username,
						password: msg.data.password || "1234"
					};
					chrome.storage.local.set({"pool": setting});
					break;
			}
	});

	chrome.app.window.create("index.html", {
		innerBounds: {
		//bounds: {
			width: 870,
			height: 870,
			minWidth: 870,
			minHeight: 870
		},
		id: "Avalon Nano"
	});
});
