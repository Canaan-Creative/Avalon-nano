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


function main() {
	setInterval(function() {
		
		var xhr = new XMLHttpRequest();
		xhr.open("GET", 'https://data.btcchina.com/data/ticker?maket=all', true);

		xhr.onreadystatechange = function() {
			if (xhr.readyState == 4) {
				message = JSON.parse(xhr.responseText);
				$("#btc-price").html(message.ticker.sell);
			}
		};
		xhr.send();

	} , 1000*5);

	setInterval(function(){
		chrome.storage.local.get('pool' , function(result){
			if(typeof(result.pool) != "undefined"){
				pool_address = result.pool.address;
				pool_worker= result.pool.worker;
				pool_password = '123456';
				$.httpRequest._connect(pool_worker, pool_password , pool_address);
			}else{
				console.error('Pleace set pool');		
			}
		});
	} , 1000);

	setInterval(function() {
		var data = nanos[0].get_data();
		if(data !== undefined){
			var work_data = work_list.shift();
			if (data.type === P_NONCE) {
				var nonce = data.nonce.toString(16);
				var pad = "00000000";
				var nonce = pad.substring(0, pad.length - nonce.length) + nonce;
				console.log("Nonce : ", nonce );
				nonce_list.push(nonce);
					
				new_nonce = '';
				new_nonce = nonce.substr(6,2)+nonce.substr(4,2)+nonce.substr(2,2)+nonce.substr(0,2);
				var solution = work_data.slice(0,152)+new_nonce+work_data.slice(160,256);
				$.httpRequest._connect(pool_worker, pool_password , pool_address , solution);
			}
			
		}
	}, 1000);

	setTimeout(function() {
		for (var nano of nanos)
			nano.sync_run(4);
	}, 1000);

	setTimeout(function() {
		for (var nano of nanos)
			nano.stop();
	}, 60 * 24 * 60000 + 1000);

	setTimeout(function() {
		for (var nano of nanos)
			nano.disconnect();
	}, 10000 + 60 * 24 * 60000 + 1000);
}

jQuery.httpRequest = {
	_connect : function (username , password ,  url ,data) {
		if(!username && !password && !url)
			return -1;

		var authpair = username + ":" + password;
		var authhdr = "Basic " + base64encode(authpair);
		var method = 'getwork';

		var params = data ? [data] : [];
		var obj = {
			'version' : '1.1' , 
			'method' : method , 
			'id' : 10 , 
			'params' : params
		}
		obj = JSON.stringify(obj);
	
		var xhr = new XMLHttpRequest();
		xhr.open("POST", url, true);
		xhr.setRequestHeader("Authorization" , authhdr );
		xhr.setRequestHeader("Content-type" , "application/json");

		xhr.onreadystatechange = function() {
			if (xhr.readyState == 4) {
				message = JSON.parse(xhr.responseText);
				if(message.error == null){
					if((typeof(message.result.data) != 'undefined') && (typeof(message.result.midstate) != 'undefined')){
						var raw = gw_pool2raw(message.result.midstate, message.result.data);
						work_list.push(message.result.data);
						nanos[0].push_data(raw);
					}else{
						var nonce = nonce_list.shift();
						$("#debug-info").val('Subminting ' + nonce+"\r\n"+$("#debug-info").val());
					}
				}else{
					console.error(message.error);
				}
			}
		};
		xhr.send(obj);
	}	

};

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

var TEST2 = [
    0xf6, 0xcd, 0x53, 0x45, 0xf8, 0xde, 0x06, 0x4e,
    0xc4, 0xb0, 0x40, 0x1a, 0x2f, 0xdf, 0x2b, 0x84,
    0x50, 0xe1, 0x23, 0xa1, 0x7c, 0x25, 0xbf, 0xf6,
    0x91, 0xdc, 0x24, 0x6c, 0x50, 0xe7, 0x15, 0x5e,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x71, 0x3c, 0x16, 0x18,
    0xce, 0x78, 0x30, 0x55, 0x2c, 0x51, 0xfb, 0xb8
];

var raw = gw_pool2raw(test.midstat, test.data);
//var raw = new Uint8Array(TEST2).buffer;

var nanos = [];
var work_list = [];
var nonce_list = [];
var MHz = 0;
init(main);
//getWork();
