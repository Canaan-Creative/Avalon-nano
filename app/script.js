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
			nano.run(5);
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
var raw = gw_pool2raw(test.midstat, test.data);

var nanos = [];
init(main);
//getWork();
