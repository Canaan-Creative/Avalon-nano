var miner = new Miner();
miner.setPool({
	url: 'stratum.btcchina.com',
	port: 3333,
	username: "canaan.apptest",
	password: "1234"
}, 0);
var MHz = 0;

miner.onNewNano.addListener(function(info) {
	console.log('!!!Event!!! New Nano');
});

miner.onNanoDeleted.addListener(function(nanoId) {
	console.log('!!!Event!!! Nano Deleted');
});

miner.onNanoConnected.addListener(function(info) {
	console.log('!!!Event!!! Nano Connected');
});

miner.onNanoDetected.addListener(function(info) {
	console.log('!!!Event!!! Nano Detected');
});

miner.onNewNonce.addListener(function(info) {
	console.log('!!!Event!!! New Nonce');
});

miner.onNewStatus.addListener(function(info) {
	console.log('!!!Event!!! New Status');
});

miner.onPoolSubscribed.addListener(function(info) {
	console.log('!!!Event!!! Pool Subscribed');
});

miner.onPoolAuthorized.addListener(function(info) {
	console.log('!!!Event!!! Pool Authorized');
});
