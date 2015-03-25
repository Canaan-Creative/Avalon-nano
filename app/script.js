function init() {
	chrome.hid.getDevices(FILTERS, function(devices) {
		if (chrome.runtime.lastError) {
			console.log(chrome.runtime.lastError);
			return;
		}
		for (device of devices) {
			chrome.hid.connect(device.deviceId, function(connection) {
				if (chrome.runtime.lastError) {
					console.log(chrome.runtime.lastError);
					return;
				}
				nanos.push(new Nano(device, connection));
			});
		}
	});
}
var nanos = [];
init();
setTimeout(function() {
	nanos[0].detect();
}, 1000);
