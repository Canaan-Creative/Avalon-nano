function init(callback) {
	chrome.hid.getDevices(FILTERS, function(devices) {
		if (chrome.runtime.lastError) {
			console.error(chrome.runtime.lastError.message);
			return;
		}
		for (var device of devices)
			nanos.push(new Nano(device));
		for (var nano of nanos)
			nano.connect();
		callback();
	});
}

chrome.hid.onDeviceRemoved.addListener(function(deviceId) {
	if(chrome.runtime.lastError) {
		console.error(chrome.runtime.lastError.message);
	}
	console.log('onDeviceRemoved' + deviceId.deviceid);
});

function getWork() {
	var xhr = new XMLHttpRequest();
	xhr.open("GET", "http://t.local.com", true);
	xhr.onreadystatechange = function() {
		if (xhr.readyState == 4) {
			console.log('xxxxxx:' + xhr.responseText);
		}
	};
	xhr.send();
}

function main() {
	setInterval(function() {
		for (var nano of nanos)
			nano.push_data(raw);
	}, 20);

	setTimeout(function() {
		for (var nano of nanos)
			nano.old_run(4);
	}, 1000);

	setTimeout(function() {
		for (var nano of nanos)
			nano.stop();
	}, 10 * 60000 + 1000);

	setTimeout(function() {
		for (var nano of nanos)
			nano.disconnect();
	}, 10000 + 10 * 60000 + 1000);
}

var test = {
	data: (
		'000000026176df7a' +
		'9c0a2997d206f06d' +
		'ce3eab91f3b74b7f' +
		'1065196300000000' +
		'0000000076e0e1b9' +
		'8f3bac97f305f2e9' +
		'569f2386ee32fd91' +
		'c274fa135d39260b' +
		'3a15086855279543' +
		'18163c7100000000' +
		'0000008000000000' +
		'0000000000000000' +
		'0000000000000000' +
		'0000000000000000' +
		'0000000000000000' +
		'0000000080020000'
	),
	midstat: (
		'5dce4b04e57b2f89' +
		'7628683e6a350fc8' +
		'e64629dad458ab4a' +
		'f08b3339845f9056'
	)
};

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


//var raw = gw_pool1raw(test.midstat, test.data);
var raw = new Uint8Array(TEST).buffer;

var nanos = [];
init(main);
//getWork();
