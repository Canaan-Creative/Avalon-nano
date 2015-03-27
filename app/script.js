function init() {
	chrome.hid.getDevices(FILTERS, function(devices) {
		if (chrome.runtime.lastError) {
			console.error(chrome.runtime.lastError);
			return;
		}
		var register = function(connection) {
			if (chrome.runtime.lastError) {
				console.error(chrome.runtime.lastError);
				return;
			}
			nanos.push(new Nano(device, connection));
		};
		for (var device of devices)
			chrome.hid.connect(device.deviceId, register);
	});
}

var TEST = [
	0x17, 0x8a, 0xb1, 0x9c, 0x1e, 0x0d, 0xc9, 0x65,
	0x1d, 0x37, 0x41, 0x8f, 0xbb, 0xf4, 0x4b, 0x97,
	0x6d, 0xfd, 0x45, 0x71, 0xc0, 0x92, 0x41, 0xc4,
	0x95, 0x64, 0x14, 0x12, 0x67, 0xef, 0xf8, 0xd8,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0xd0, 0xc1, 0x4a,
	0x50, 0x70, 0x51, 0x88, 0x1a, 0x05, 0x7e, 0x08
];

var nanos = [];
init();

setInterval(function() {
	if (nanos[0].out_buffer.length < nanos[0].BUFFER_SIZE)
		nanos[0].push_data(new Uint8Array(TEST).buffer);
}, 10);

setTimeout(function() {
	nanos[0].run(5);
}, 1000);

setTimeout(function() {
	nanos[0].stop = true;
}, 60000 + 1000);

setTimeout(function() {
	chrome.hid.disconnect(nanos[0].connection.connectionId, function() {
		console.info("Disconnected.");
	});
}, 10000 + 60000 + 1000);
