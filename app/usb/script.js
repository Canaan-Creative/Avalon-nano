var device_selector = document.getElementById('device-selector');
var add_device = document.getElementById('add-device');
var device_info = document.getElementById('device-info');
var devices = {};

function appendToDeviceSelector(device) {
	var el = document.createElement('option');
	el.setAttribute('value', device.device);
	el.textContent =
	'Product 0x' + ('0000' + device.productId.toString(16)).slice(-4) +
	' Vendor 0x' + ('0000' + device.vendorId.toString(16)).slice(-4);
	console.log(el);
	device_selector.add(el);
};

function appendDeviceInfo(name, value) {
	var el = document.createElement('b');
	el.textContent = name + ': '
	device_info.appendChild(el);
	device_info.appendChild(document.createTextNode(value));
	device_info.appendChild(document.createElement('br'));
}

function populateDeviceInfo(handle, callback) {
	console.log("callback function : ");
	console.log(callback);
	chrome.usb.getConfiguration(handle, function(config) {
		console.log('populateDeviceInfo start');
		console.log(config);

		if (chrome.runtime.lastError != undefined) {
			var el = document.createElement('em');
			el.textContent = 'Failed to read device configuration: ' +
			chrome.runtime.lastError.message;
			device_info.appendChild(el);
		} else {
			var el = document.createElement('h2');
			//The configuration number | config.configruationValue
			el.textContent = 'Configuration ' + config.configurationValue;
			device_info.appendChild(el);
			//Available interfaces. | config.interfaces 
			for (var iface of config.interfaces) {
				el = document.createElement('h3');
				el.textContent = 'Interface ' + iface.interfaceNumber;
				device_info.appendChild(el);
				//The interface alternate setting number (defaults to 0
				appendDeviceInfo('Alternate Setting', iface.alternateSetting);
				//The USB interface class. 
				appendDeviceInfo('Inteface Class', iface.interfaceClass);
				//The USB interface sub-class. 
				appendDeviceInfo('Interface Subclass', iface.interfaceSubclass);
				//The USB interface protocol. 
				appendDeviceInfo('Interface Protocol', iface.interfaceProtocol);
				
				//Available endpoints.  | iface.endpoints
				for (var endpoint of iface.endpoints) {
					el = document.createElement('h4');
					//Endpoint address
					el.textContent = 'Endpoint ' + endpoint.address;
					device_info.appendChild(el);
					//Transfer type 	
					appendDeviceInfo('Type', endpoint.type);
					//Transfer direction
					appendDeviceInfo('Direction', endpoint.direction);
					//Maximum packet size
					appendDeviceInfo('Maximum Packet Size', endpoint.maximumPacketSize);
				}
			}
		}

		//callback() : chrome.usb.closeDevice(handle)
		callback();
	});
}

function deviceSelectionChanged() {
	device_info.innerHTML = "";
	console.log("devSelectionChanged start");
	console.log(device_selector);
	//选中的索引 :索引从0开始
	var index = device_selector.selectedIndex;
	console.log('SelectionChanged:'+ index);
	if (index == -1) {
		var el = document.createElement('em');
		el.textContent = 'No device selected.';
		device_info.appendChild(el);
	} else {
		
		//当前选中的对象		
		var device = devices[device_selector.options.item(index).value];
		console.log('-------------------------');
		console.log(device);
		appendDeviceInfo(
		'Product ID',
		'0x' + ('0000' + device.productId.toString(16)).slice(-4));
		appendDeviceInfo(
		'Vendor ID',
		'0x' + ('0000' + device.vendorId.toString(16)).slice(-4));

		chrome.usb.openDevice(device, function(handle) {
			console.log(chrome.runtime.lastError);
			if (chrome.runtime.lastError != undefined) {
				var el = document.createElement('em');
				el.textContent = 'Failed to open device: ' +
				chrome.runtime.lastError.message;
				device_info.appendChild(el);
			} else {
				console.log('Open device success : ');
				console.log(handle);
				populateDeviceInfo(handle, function () {
					chrome.usb.closeDevice(handle);
				});
			}
		});
	}
}

chrome.usb.getDevices({}, function(found_devices) {
	if (chrome.runtime.lastError != undefined) {
		console.warn('chrome.usb.getDevices error: ' +
		chrome.runtime.lastError.message);
		return;
	}
	
	/*found_devices is [] */
	if(found_devices.length){	
		console.log("Find "+found_devices.length+" Devices");
	for (var device of found_devices) {
		console.log('fdsafdsa');
		devices[device.device] = device;
		appendToDeviceSelector(device);
	}
	}else {
		console.log('No Found Devices!');
	}
});
/*
chrome.usb.onDeviceAdded.addListener(function (device) {
	console.log('aaaaaaaaaaaaaaaaaaaaaaaaaaaaa');
});
*/

if (chrome.usb.onDeviceAdded) {
	console.log('************onDeviceAdded*********');
	chrome.usb.onDeviceAdded.addListener(function (device) {
		devices[device.device] = device;
		appendToDeviceSelector(device);
	});
}
/*
chrome.usb.onDeviceRemoved.addListener(function (device) {
	console.log('rrrrrrrrrrrrrrrrrrrrrrrrrr');
});
*/

if (chrome.usb.onDeviceRemoved) {
	console.log('##################onDeviceRemoved#################');
	chrome.usb.onDeviceRemoved.addListener(function (device) {
		delete devices[device.device];
		for (var i = 0; i < device_selector.length; ++i) {
			if (device_selector.options.item(i).value == device.device) {
				device_selector.remove(i);
				deviceSelectionChanged();
				break;
			}
		}
	});
}

add_device.addEventListener('click', function() {

	chrome.usb.getUserSelectedDevices({
		'multiple': false  //是否开启多选 
		}, function(selected_devices) {

		if (chrome.runtime.lastError != undefined) {
			console.warn('chrome.usb.getUserSelectedDevices error: ' +
			chrome.runtime.lastError.message);
			return;
		}

		for (var device of selected_devices) {
			console.log(device);
			console.log('LOG:' + device);
			var deviceInfo = { 'device': device, 'index': undefined };
			console.log(device.device);
			console.log(devices);
			//判断是否已经在左侧列表中
			if (device.device in devices) {
				console.log('In');
				for (var i = 0; i < device_selector.length; ++i) {
					if (device_selector.options.item(i).value == device.device) {
						device_selector.selectedIndex = i;
						break;
					}
				}
			} else {
				console.log('Not in');
				devices[device.device] = device;
				appendToDeviceSelector(device);
				device_selector.selectedIndex = device_selector.options.length - 1;
			}
			deviceSelectionChanged();
		}
	});
});

