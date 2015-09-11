var hashrate = [0, 0, 0, 0];
var enterFlag = false;
var pools = [];
var devices = [];
var param = [];
var highcharts;

var renderChart = function() {
	Highcharts.setOptions({
		global: {useUTC: false}
	});
	$('#container').highcharts({
		chart: {
			type: 'spline',
			animation: Highcharts.svg, // don't animate in old IE
			marginRight: 10,
		},
		title: {text: chrome.i18n.getMessage("chartTitle")},
		xAxis: {
			type: 'datetime',
			tickPixelInterval: 150,
		},
		yAxis: {
			title: {text: 'Hash/s'},
			min: 0,
			plotLines: [{value: 0, width: 1, color: '#808080'}],
		},
		tooltip: {
			formatter: function() {
				var s = Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', this.x);
				s += '<table>';
				for (var i in this.points)
					s += '<tr style="color: ' + this.points[i].series.color + '"><td><b>' +
						this.points[i].series.name + '</b>:&nbsp;</td><td>' +
						numberShorten(this.points[i].y) + 'Hs</td></tr>';
				return s + '</table>';
			},
			useHTML: true,
			shared: true,
		},
		credits: {enabled: false},
		legend: {
			enabled: true,
			layout: 'vertical',
			align: 'right',
			verticalAlign: 'top',
			borderWidth: 0,
		},
		exporting: {enabled: false},
		series: (function() {
			var series = [];
			var names = [
				chrome.i18n.getMessage("chartSecond"),
				chrome.i18n.getMessage("chartMinute"),
				chrome.i18n.getMessage("chartHour"),
				chrome.i18n.getMessage("chartPool"),
			];
			var colors = ['#808080', '#feae1b', '#00cc99', '#0066FF'];
			var data = [];
			var time = (new Date()).getTime();
			for (var i = -899; i <= 0; i++) {
				data.push({
					x: time + i * 1000,
					y: 0,
				});
			}
			for (i in names)
				series.push({
					name: names[i],
					color: colors[i],
					data: data,
				});
			return series;
		})()
	});
	highcharts = $('#container').highcharts();
};

var getGlobalData = function() {
		var htmlObj;
		$.ajax({
			url: 'https://bitcoinwisdom.com/',
			type: "GET",
			success: function(htmlData) {
				htmlObj = $(htmlData);
				$("#price").html($("#market_huobibtccny", htmlObj).html());
				$("#difficulty").html($("#slot_difficulty", htmlObj).html());
				$("#nextdiff").html($("#slot_estimated", htmlObj).html());
				$("#global-hashrate").html($("#slot_hash_rate", htmlObj).html());
			}
		});
};

var getPoolHashrate = function() {
	setInterval(function() {
		var poolId = $("#pool-info tr:eq(1)").data('id');
		if (poolId !== undefined) {
			var address = $("#pool-address-" + poolId).html();
			var worker = $("#pool-worker-" + poolId).html();
			var api_key = $("#pool-api-key-" + poolId).val().trim();

			if (address.indexOf('btcchina') > 0 && api_key.length !== 0) {
				var url = "https://pool.btcchina.com/api?api_key=" + api_key;
				$.ajax({
					url: url,
					contentType: "application/json; charset=UTF-8",
					dataType: "json",
					success: function(msg) {
						var workers = msg.user.workers;
						if (workers !== undefined) {
							for(var data of workers){
								if (data.worker_name === worker)
									hashrate[3] = data.hashrate;
							}
						}
					}
				});
			}
		}
	}, 5000);
};

var numberShorten = function(num) {
	var prefix = [
		{prefix: 'E', base: 1000000000000000000},
		{prefix: 'P', base: 1000000000000000},
		{prefix: 'T', base: 1000000000000},
		{prefix: 'G', base: 1000000000},
		{prefix: 'M', base: 1000000},
		{prefix: 'k', base: 1000},
	];
	for (var p of prefix)
		if (num > p.base)
			return (num / p.base).toFixed(3) + ' ' + p.prefix;
	return num + ' ';
};

var updateHashrate = function(hashrates) {
	var h = hashrates[hashrates.length - 1];
	hashrate[0] = h.hs5s;
	hashrate[1] = h.hs1m;
	hashrate[2] = h.hs1h;
	for(var i = 0; i < hashrates.length - 1; i++){
		updateDeviceGHs5s(hashrates[i].deviceId, (hashrates[i].hs5s / 1000000000).toFixed(1));
	}

	var x = (new Date()).getTime();
	var series = highcharts.series;
	for (i in series)
		series[i].addPoint([x, hashrate[i]], true, true);
};

var addDevice = function(device) {
	devices.push(device);
	if (enterFlag) {
		addDeviceRow(device);
	} else {
		$(".detect p img").attr("src", "images/loading-m.gif");
		setTimeout(function() {
			detectDevice();
		}, 900);
	}
};

var detectFlag = false;
var detectDevice = function() {
	if (!enterFlag) {
		if (devices.length < 1) {
			$(".detect p").html('<img src="images/device.png">');
			detectFlag = false;
		}
		if (!detectFlag) {
			if(devices.length >= 1 ){
				detectFlag = true;
				$(".detect p img").remove();
				$(".detect p").append(`
					<div style="text-align:center;">
						<button type="button" data-type="enter" class="btn btn-default">${chrome.i18n.getMessage("enter")}</button>&nbsp;
						<button type="button" data-type="pool-setting" class="btn btn-default">${chrome.i18n.getMessage("poolSetting")}</button>
					</div>`);

				$("p").on("click", "button", function() {
					enterFlag = true;
					switch ($(this).data('type')) {
					case 'enter':
						mainPage();
						break;
					case 'pool-setting':
						dialog("pool");
						bindSavePoolSetting(mainPage);
						break;
					}
				});
			}
		}
	}
};

var initPools = function(data) {
	if (data !== undefined || data !==null) {
		for (var id in data){
			if (data[id] !== undefined && data[id] !== null)
				pools.push(data[id]);
		}
	}
};

var initParam = function(data) {
	if (data.voltSet !== undefined && data.freqSet !== undefined)
		param = data;
	else
		param = {voltSet: 6500, freqSet: [100, 100, 100]};
};

var loadingImg = "<img id='loadImg' src='images/loading.gif'/>";

var guidePage = function() {
	var main = $("#main");
	main.addClass('center-img').append(loadingImg);
	setTimeout(function() {
		$("#loadImg").remove();
		main.removeClass('center-img');
		main.append(`
			<div class="guide">
				<div class="logo">
					<img src="images/logo.png"/>
				</div>
				<div class="detect">
					<p><img src="images/loading-m.gif"/></p>
					<h4>${chrome.i18n.getMessage("welcomeGuide")}</h4>
					<br />
					<h5>${chrome.i18n.getMessage("supportList")}</h5>
					<br />
					<h5>${chrome.i18n.getMessage("nonRecognized")}</h5>
					<h5>${chrome.i18n.getMessage("contactUs")}</h5>
					<br />
					<h4 style="color:red;font-weight:bold">&#9888; ${chrome.i18n.getMessage("warningTitle")}</h4>
					<h5 style="color:red;font-weight:bold">${chrome.i18n.getMessage("warningMessage1")}</h5>
					<h5 style="color:red;font-weight:bold">${chrome.i18n.getMessage("warningMessage2")}</h5>
				</div>
				<p>${chrome.runtime.getManifest().name} v${chrome.runtime.getManifest().version}<br />${chrome.i18n.getMessage("poweredBy")}</p>
			</div>`);
		setTimeout(function() {
			$(".detect p img").attr("src", "images/device.png");
			chrome.runtime.sendMessage({type: "ready"});
		}, 500);
	}, 500);
};

var mainPage = function() {
	enterFlag = true;
	var mainObj = $("#main");
	mainObj.addClass('center-img').html(loadingImg);
	setTimeout(function() {
		$("#loadImg").remove();
		mainObj.removeClass('center-img');
		mainObj.html(topPart() + chartPart() + tablePart() + bottomPart());
		renderChart();
		bindPoolSetting();
		bindDeviceSetting();
		for(var i of devices)
			addDeviceRow(i);
		chrome.runtime.sendMessage({type: "start"});
	}, 500);
	getGlobalData();
	setInterval(getGlobalData, 1000 * 5);
	getPoolHashrate();
};

var bottomPart = function() {
	return `
		<div class="login-panel">
			<div class="panel-center">
				<p class="tips">Canaan Creative</p>
				<p>
					<a title="EHash" href="https://ehash.com" target="_blank" rel="nofollow"><img alt="EHash" src="images/ehash.png"></a>
					<a title="Canaan Creative" href="http://www.canaan-creative.com" target="_blank" rel="nofollow"><img alt="Canaan Creative" src="images/home.png"></a>
					<a title="#Avalon" href="http://webchat.freenode.net/?randomnick=1&channels=avalon" target="_blank" rel="nofollow"><img alt="#avalon" src="images/irc.png"></a>
					<a title="service@canaan-creative.com" href="mailto:service@canaan-creative.com" target="_blank" rel="nofollow"><img alt="service@canaan-creative.com" src="images/email.png"></a>
				</p>
			</div>
		</div>`;
};

var bindDeviceSetting = function() {
	$(".device-cfg").on("click", function() {
		dialog("device");
		bindSaveDeviceSetting();
	});
};

var bindSaveDeviceSetting = function() {
	$("#save-device-setting").on("click", function() {
		var volt = parseInt($("#voltage-input").val());
		var freq1 = parseInt($("#frequency1-input").val());
		var freq2 = parseInt($("#frequency2-input").val());
		var freq3 = parseInt($("#frequency3-input").val());
		volt = (isNaN(volt) || volt > 8000) ? 6500 : volt;
		freq1 = (isNaN(freq1) || freq1 > 400) ? 100 : freq1;
		freq2 = (isNaN(freq2) || freq2 > freq1) ? freq1 : freq2;
		freq3 = (isNaN(freq3) || freq3 > freq2) ? freq2 : freq3;
		param = {freqSet: [freq1, freq2, freq3], voltSet: volt};
		chrome.runtime.sendMessage({type: "setting", param: param});
		$('#dialogModel').modal('hide');
	});
};

var bindPoolSetting = function() {
	$(".pool-cfg").on("click", function() {
		dialog("pool");
		bindSavePoolSetting();
	});
};

var bindSavePoolSetting = function(callback) {
	$("#save-pool-setting").on("click", function() {
		pools = [];
		for (var i = 0; i < 3 ; i++) {
			var address = '';
			var worker = '';
			var api_key = '';
			address = $("#pool-setting-address-" + i).val();
			worker = $("#pool-setting-worker-" + i).val();
			api_key = $("#pool-setting-api-key-" + i).val();
			if (!address && !worker && !api_key)
				continue;
			if (address.indexOf("://") >= 0)
				address = address.split("://")[1];
			var temp = address.split(":");
			pools.push({
				address: temp[0],
				port: parseInt(temp[1] || 3333),
				username: worker,
				api_key: api_key,
			});
		}
		chrome.runtime.sendMessage({type: "setting", pool: pools});
		updatePoolList();
		$('#dialogModel').modal('hide');
		if (callback !== undefined) {
			callback();
		}
	});
};

var removeDevice = function(deviceId) {
	$("#device-tr-id-" + deviceId).remove();
	var temp = [];
	for(var i of devices){
		if (i.deviceId === deviceId)
			continue;
		temp.push(i);
	}
	devices = temp;
	if (!enterFlag)
		detectDevice();
	if ($('#device tr').size() === 1)
		$("#device").append('<tr id="device-null"><td colspan="5" align="center" style="color:#cfcfcf;">' + chrome.i18n.getMessage("inserUSB") + '</td></tr>');
};

var topPart = function() {
	return `
		<div class="row top-div">
			<ul>
				<li><p>${chrome.i18n.getMessage("price")} (USD/CNY): <span id="price">0</span></p></li>
				<li><p>${chrome.i18n.getMessage("diff")}: <span id="difficulty">0</span></p></li>
				<li><p>${chrome.i18n.getMessage("nextDiff")}: <span id="nextdiff">0</span></p></li>
				<li><p>${chrome.i18n.getMessage("globalHashrate")}: <span id="global-hashrate">0</span></p></li>
			</ul>
		</div>`;
};

var chartPart = function() {
	return '<div id="container" style="min-width:300px;height:400px"></div>';
};

var panelPart = function(type, position, title) {
	var partTpl, col;
	if (type === 'device') {
		partTpl = devicePart();
		col = 6;
	} else {
		partTpl = poolPart();
		col = 6;
	}
	return `
		<div class="col-xs-${col} col-md-${col} pull-${position}">
			<div class="panel panel-default">
				<div class="panel-heading">
					<h3 class="panel-title">${title}
						<span class="glyphicon glyphicon-cog pull-right ${type}-cfg" aria-hidden="true"></span>
					</h3>
				</div>
				<div class="panel-body" id="${type}-list">
					${partTpl}
				</div>
			</div>
		</div>`;
};

var updatePoolList = function() {
	$("#pool-list").html(poolPart());
	bindPoolSetting();
};

var updatePoolStatus = function(poolInfo) {
	$("#pool-status-" + poolInfo.poolId).html(
		`<p class='pool-status-${poolInfo.info}'>${chrome.i18n.getMessage(poolInfo.info)}</p>`
	);
};

var updateDeviceStatus = function(device) {
	if (device.temperatureCu !== undefined)
		updateDeviceTemp(device.deviceId, device.temperatureCu, 'cu');
	if (device.temperatureFan !== undefined)
		updateDeviceTemp(device.deviceId, device.temperatureFan, 'fan');
	if (device.temperature !== undefined)
		updateDeviceTemp(device.deviceId, device.temperature, 'all');
	if (device.version !== undefined)
		updateDeviceVersion(device.deviceId, device.version);
};

var updateDeviceVersion = function(deviceId, version) {
	$("#device-version-" + deviceId).html(version);
};

var updateDeviceGHs5s = function(deviceId, data) {
	$("#device-ghs5s-" + deviceId).html(data);
};

var updateDeviceTemp = function(deviceId, temp, type) {
	$("#" + type + "-temp-" + deviceId).html(temp);
};

var devicePart = function() {
	return `
	<table class="table table-hover" id="device">
		<tr>
			<th>${chrome.i18n.getMessage("device")}</th>
			<th>ID</th>
			<th>${chrome.i18n.getMessage("version")}</th>
			<th>${chrome.i18n.getMessage("temp")}(&deg;C)</th>
			<th>GHs5s</th>
		</tr>
	</table>`;
};

var poolPart = function() {
	var content = `
	<table class="table table-hover" id="pool-info">
		<tr>
			<th>${chrome.i18n.getMessage("address")}</th>
			<th>${chrome.i18n.getMessage("worker")}</th>
			<th>${chrome.i18n.getMessage("status")}</th>
		</tr>`;

	if (pools.length)
		for (var id in pools)
			if (pools[id] !== undefined && pools[id] !== null)
				content += getPoolRow(id);

	content += '</table>';
	return content;
};

var getPoolRow = function(id) {
	return `
		<tr data-id="${id}" id="pool-tr-id-${id}" class="pool-tr-line">
			<td id="pool-address-${id}">${pools[id].address}:${pools[id].port}</td>
			<td id="pool-worker-${id}">${pools[id].username}</td>
			<td id="pool-status-${id}">--</td>
			<input type="hidden" id="pool-api-key-${id}" value="${pools[id].api_key}"/>
		</tr>`;
};

var addDeviceRow = function(device) {
	if ($("#device-null").html() !== undefined)
		$("#device-null").remove();

	var temp;
	switch (device.deviceType) {
	case 'Avalon nano':
		temp = `<td id="device-temp-${device.deviceId}"><span id="all-temp-${device.deviceId}">0</span></td>`;
		break;
	case 'Avalon4 mini':
		temp = `<td id="device-temp-${device.deviceId}"><span id="cu-temp-${device.deviceId}">0</span> / <span id="fan-temp-${device.deviceId}">0</span></td>`;
		break;
	}

	$("#device tr:last").after(`
		<tr class="active" id="device-tr-id-${device.deviceId}">
			<td>${device.deviceType}</td>
			<td id="device-device-id-${device.deviceId}">${device.deviceId}</td>
			<td id="device-version-${device.deviceId}">---</td>
			${temp}
			<td id="device-ghs5s-${device.deviceId}">0</td>
		</tr>`);
};

var tablePart = function() {
	return `
		<div class="row" style="margin-bottom:50px;">
			${panelPart('device', 'left', chrome.i18n.getMessage("deviceList"))}${panelPart('pool', 'right', chrome.i18n.getMessage("poolList"))}
		</div>`;
};

var dialog = function(type) {
	var title, content;
	if (type === "pool") {
		title = chrome.i18n.getMessage("poolSetting");
		content  = `
			<div>
				<div class="form-group">
					<table class="table">
						<tr>
							<th></th>
							<th>${chrome.i18n.getMessage("address")}</th>
							<th>${chrome.i18n.getMessage("worker")}</th>
							<th>API ${chrome.i18n.getMessage("key")}</th>
						</tr>`;
		for(var i= 0 ; i < 3 ; i++){
			var address = '';
			var worker = '';
			var api_key = '';
			var port = '';
			if (pools[i] !== undefined) {
				address = pools[i].address || '';
				worker = pools[i].username || '';
				api_key = pools[i].api_key || '';
				port = pools[i].port || '';
				address = address + ':' + port;
			}
			content += `
				<tr>
					<td><label for="">#${i + 1}</label></td>
					<td><input type="text" class="form-control" value="${address}" id="pool-setting-address-${i}" placeholder=" "></td>
					<td><input type="text" class="form-control" value="${worker}" id="pool-setting-worker-${i}" placeholder=" "></td>
					<td><input type="text" class="form-control" value="${api_key}" id="pool-setting-api-key-${i}" placeholder=" "></td>
				</tr>`;
		}
		content += `
						<tr><td colspan="4"><div class="form-group">
							<button typ="button" class="btn btn-default pull-right" id="save-pool-setting">${chrome.i18n.getMessage("save")}</button>
							<span id="message"></span>
						</div></td></tr>
					</table>
				</div>
			</div>`;
	} else {
		title = chrome.i18n.getMessage("configuration");
		content = `
			<div class="row" style="margin-bottom:50px;">
				<div class="row col-xs-12">
					<div class="panel panel-default">
						<div class="panel-heading">
							<h3 class="panel-title">Avalon4 mini</h3>
						</div>
						<div class="panel-body" id="setting-table">
							<table class="table" style="margin-bottom:0px;">
								<tr>
									<th>${chrome.i18n.getMessage("voltage")}</th>
									<th>${chrome.i18n.getMessage("frequency")}1</th>
									<th>${chrome.i18n.getMessage("frequency")}2</th>
									<th>${chrome.i18n.getMessage("frequency")}3</th>
								</tr>
								<tr>
									<td><input type="text" class="form-control" id="voltage-input" value="${param.voltSet}" placeholder=" "></td>
									<td><input type="text" class="form-control" id="frequency1-input" value="${param.freqSet[0]}" placeholder=" "></td>
									<td><input type="text" class="form-control" id="frequency2-input" value="${param.freqSet[1]}" placeholder=" "></td>
									<td><input type="text" class="form-control" id="frequency3-input" value="${param.freqSet[2]}" placeholder=" "></td>
								</tr>
							</table>
						</div>
					</div>
				</div>
				<div class="form-group">
					<button type="button" class="btn btn-default pull-right" id="save-device-setting">${chrome.i18n.getMessage("save")}</button>
				</div>
			</div>`;
	}

	if ($("#dialogModel").length > 0) {
		$("#dialogModel").remove();
		$(".modal-backdrop").remove();
	}

	$("body").append(`
		<div class="modal" id="dialogModel">
			<div class="modal-dialog modal-lg">
				<div class="modal-content">
					<div class="modal-header">
						<button type="button" class="close" data-dismiss="modal" aria-hidden="true">&times;</button>
						<h4 class="modal-title"><strong>${title}</strong></h4>
					</div>
					<div class="modal-body">${content}</div>
				</div>
			</div>
		</div>`);

	$('#dialogModel').modal({
	      backdrop: 'static',
	      keyboard: false,
	});
};

$(function() {
	guidePage();
	chrome.runtime.onMessage.addListener(function(msg, sender) {
		if (sender.url.indexOf('background') > -1)
			switch (msg.type) {
			case "setting":
				initParam(msg.param);
				initPools(msg.pool);
				break;
			case "device":
				addDevice({deviceId: msg.deviceId, deviceType: msg.deviceType});
				break;
			case "delete":
				removeDevice(msg.deviceId);
				break;
			case "pool":
				updatePoolStatus(msg);
				break;
			case "status":
				updateDeviceStatus(msg);
				break;
			case "hashrate":
				updateHashrate(msg.hashrate);
				break;
			}
	});
});
