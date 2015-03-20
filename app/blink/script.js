var AVALON_NANO_VENDOR_ID = 10737; //0x29f1;
var AVALON_NANO_PRODUCT_ID = 13297; //0x33f1;
var DEVICE_INFO = {
	"vendorId": AVALON_NANO_VENDOR_ID,
	"productId": AVALON_NANO_PRODUCT_ID
}

var TEST_OUT_DATA = {
	"direction": "out",
	"endpoint": 1,
	"data": new Uint8Array([
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
	]).buffer
}

var TEST_IN_DATA = {
	"direction": "in",
	"endpoint": 1,
	"length": 4
}

var IR_DATA = {
	"direction": "out",
	"endpoint": 1,
	"data": new Uint8Array([
        'c'.charCodeAt(0),
        255, 0, 0,
        0, 0, 0
    ]).buffer
}

//chrome.usb.getDevices(DEVICE_INFO, function(devices) {
//	console.log("In getting device.");
//	console.log(devices);
//	for (var device of devices) {
//		chrome.usb.openDevice(device, function(handle) {
//			console.log("In openning device.");
//			if (chrome.runtime.lastError) {
//				console.error(chrome.runtime.lastError);
//				return;
//			}
//			console.log(handle);
//			chrome.usb.closeDevice(handle);
//		})
//
//	}
//});

chrome.usb.findDevices(DEVICE_INFO, function(handles) {
	if (chrome.runtime.lastError) {
		console.log(chrome.runtime.lastError);
		return;
	}
	console.log(handles);
	for (var handle of handles) {
		//chrome.usb.getConfiguration(handle, function(config) {
		//	if (chrome.runtime.lastError) {
		//		console.log(chrome.runtime.lastError);
		//		return;
		//	}
		//	console.log(config);
		//});
		//chrome.usb.listInterfaces(handle, function(descriptors) {
		//	if (chrome.runtime.lastError) {
		//		console.log(chrome.runtime.lastError);
		//		return;
		//	}
		//	console.log(descriptors);
		//});
		//chrome.usb.resetDevice(handle, function(success) {
		//	if (chrome.runtime.lastError) {
		//		console.log(chrome.runtime.lastError);
		//		return;
		//	}
		//	console.log(success);
		//});
		//chrome.usb.releaseInterface(handle, 1, function() {
		//	if (chrome.runtime.lastError) {
		//		console.error(chrome.runtime.lastError);
		//		return;
		//	}
		//	console.log("Released.");
		//});
		chrome.usb.claimInterface(handle, 0, function() {
			if (chrome.runtime.lastError) {
				console.log(chrome.runtime.lastError);
				return;
			}
			console.log("Claimed.");
		});
		//chrome.usb.bulkTransfer(handle, TEST_OUT_DATA, function(result) {
		//	if (chrome.runtime.lastError) {
		//		console.error(chrome.runtime.lastError);
		//		return;
		//	}
		//	console.log(result);
		//});


        chrome.usb.interruptTransfer(handle, IR_DATA, function(info) {
			if (chrome.runtime.lastError) {
				console.error(chrome.runtime.lastError);
				return;
			}
			console.log(info);
        });
		chrome.usb.closeDevice(handle);
	}
});
