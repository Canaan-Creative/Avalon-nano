var miner = new Miner();
miner.setPool({
	url: 'stratum.btcchina.com',
	port: 3333,
	username: "canaan.apptest",
	password: "1234"
}, 0);
var MHz = 0;

miner.onNewNano.addListener(function(info) {});

miner.onNanoDeleted.addListener(function(nanoId) {});

miner.onNanoConnected.addListener(function(info) {});

miner.onNanoDetected.addListener(function(info) {});

miner.onNewNonce.addListener(function(info) {});

miner.onNewStatus.addListener(function(info) {});
