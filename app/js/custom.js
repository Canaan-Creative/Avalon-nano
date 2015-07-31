var hashrate = [0, 0, 0, 0];
var enterFlag = false;
var poolObj = [];
var nanoObj = [];
var paramObj = [];
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
		title: {text: 'Avalon miner Live Hashrate'},
		xAxis: {
			type: 'datetime',
			tickPixelInterval: 150
		},
		yAxis: {
			title: {text: 'Hash/s'},
			min:0,
			plotLines: [{value: 0, width: 1, color: '#808080'}]
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
			shared: true
		},
		credits: {enabled: false},
		legend: {
			enabled: true,
			layout: 'vertical',
			align: 'right',
			verticalAlign: 'top',
			borderWidth: 0
		},
		exporting: {enabled: false},
		series: (function() {
			var series = [];
			var names = ['5-Secondly', 'Minutely', 'Hourly', 'Pool'];
			var colors = ['#808080', '#feae1b', '#00cc99' , '#0066FF'];
			var data = [];
			var time = (new Date()).getTime();
			for (var i = -899; i <= 0; i++) {
				data.push({
					x: time + i * 1000,
					y:0 
				});
			}
			for (i in names)
				series.push({
					name: names[i],
					color: colors[i],
					data: data
				});
			return series;
		})()
	});
	highcharts = $('#container').highcharts();
};
var getPrice = function() {
	setInterval(getRemoteData, 1000 * 5);
}
var getRemoteData = function() {
		var htmlObj;
		$.ajax({
			url : 'https://bitcoinwisdom.com/',
			type : "GET",
			success : function(htmlData){
				htmlObj = $(htmlData);
				$("#_huobi").html($("#market_huobibtccny" , htmlObj).html());	
				$("#_difficulty").html($("#slot_difficulty" , htmlObj).html());
				$("#_next").html($("#slot_estimated" , htmlObj).html());
				$("#_hash_rate").html($("#slot_hash_rate" , htmlObj).html());
			}
		});
		remoteFlag = true;
}


var getPoolHashRate = function() {
	setInterval(function(){
		var poolId = $("#pool_info tr:eq(1)").data('id');	
		if(poolId !== undefined){
			var address = $("#pool-address-"+poolId).html(); 
			var worker = $("#pool-worker-"+poolId).html(); 
			var apiKey = $("#pool-api-key-"+poolId).val(); 

			if(address.indexOf('btcchina') > 0){
				var url = "https://pool.btcchina.com/api?api_key="+apiKey;
				$.ajax({
					url : url,
 					contentType : "application/json; charset=UTF-8",
					dataType : "json",
					success : function(msg){
						var workers = msg.user.workers;
						if(workers !== undefined){
							for(var data of workers){
								if(data.worker_name === worker)
									hashrate[3] = data.hashrate;
							}
						}
					}
				});
			}
		}
	} , 5000);
}
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

var updateHashrate = function( hashrates ){
	var h = hashrates[hashrates.length - 1];
	hashrate[0] = h.hs5s;
	hashrate[1] = h.hs1m;
	hashrate[2] = h.hs1h;
	var _len = hashrates.length -1;
	for(var i = 0; i < _len ; i++){
		updateDeviceGHs5s(hashrates[i].deviceId , (hashrates[i].hs5s/1000000000).toFixed(1));	
		updateErrorRate( hashrates[i].deviceId , (hashrates[i].errorRate * 100).toFixed(2) );
	}

	var x = (new Date()).getTime();
	var series = highcharts.series;
	for (var i in series)
		series[i].addPoint([x, hashrate[i]], true, true);

};
var nanoList = function(deviceObj) {
	nanoObj.push(deviceObj);
	if(enterFlag){
		deviceTr(deviceObj);
	}else{
		$(".detect p img").attr("src","images/loading-m.gif");                
		setTimeout(function(){
			detectDevice();
		},900);
	}
};
var detectFlag = false;
var detectDevice = function() {
	if(!enterFlag){
		if(nanoObj.length < 1){
			$(".detect p").html('<img src="images/device.png">');
			detectFlag = false;
		}
		if(!detectFlag){
			if(nanoObj.length >= 1 ){
				detectFlag = true;
				$(".detect p img").remove();	
				var btnTpl = '<div style="text-align:center;"><button type="button" data-type="enter" class="btn btn-default"> Enter </button>  ';
				if(!poolObj.length)
					btnTpl +='  <button type="button" data-type="setting-pool" class="btn btn-default"> Set Pool </button>';
				btnTpl +='</div>';
				$(".detect p").append(btnTpl);

				$("p").delegate("button","click",function(){
					enterFlag = true;
					var _type = $(this).data('type');
					switch( $(this).data('type') ) {
						case 'enter':
							mainPage();
							break;
						case 'setting-pool':
							settingPool();
							break;
					}
				});
			}	
		}

	}
}
var poolList = function(data) {
	if( data !== undefined || data !==null){
		for (var id in data){
			if (data[id] !== undefined && data[id] !== null)
				poolObj.push(data[id]);
		}
	}
};
var globalSetting = function( data ) {
	if(data.voltSet !== undefined && data.freqSet !== undefined)	
		paramObj = data;
	else
		paramObj = {voltSet:6500,freqSet:[100,100,100]};
}
var loadingImg = "<img id='loadImg' src='images/loading.gif'/>";
var guidePage = function( callback ){
	var _mainObj = $("#main");
	_mainObj.addClass('center-img').append(loadingImg);	
	setTimeout(function(){
		$("#loadImg").remove();
		_mainObj.removeClass('center-img');
		var guidTpl = `<div class="guide">
			<div class="logo">
				<img src="images/logo.png"/>
			</div>
			<div class="detect">
				<p>
					<img src="images/loading-m.gif"/>
				</p>
				<h4>To begin, plug your Avalon devices into the USB port of the computer</h4>
				<h5>If your Avalon devices are not recognized, please contact us by <a href="mailto:service@cannan-creative.com" target="_blank">service@cannan-creative.com</a></h5>
			</div>
			<p>${chrome.runtime.getManifest().name} v${chrome.runtime.getManifest().version}<br />
			Powered by <a href="http://www.canaan-creative.com" target="_blank">Canaan-Creative</a></p>
		</div>`;
		_mainObj.append(guidTpl);
		setTimeout(function(){
			$(".detect p img").attr("src","images/device.png");                
			callback();
		},500);
	},500);
}

var guideCallback = function() {
	chrome.runtime.sendMessage({type: "ready"});
}

var mainPage = function() {
	enterFlag = true;
	var _mainObj = $("#main");
	_mainObj.addClass('center-img').html(loadingImg);	
	setTimeout(function(){
		$("#loadImg").remove();
		_mainObj.removeClass('center-img');
		var _tpl = topPart();
		_tpl += chartPart();
		_tpl += settingPart();
		_tpl += tablePart(); 
		_tpl += bottom();
		_mainObj.html(_tpl);
		renderChart();
		bindPoolAdd();
		bindPoolButton();
		bindSettingBtn();
		loopNano();
		updateSetting();
		chrome.runtime.sendMessage({type: "start"});
		
	},500);
	getRemoteData();
	getPrice();
	getPoolHashRate();
	
}
var bottom = function() {
	var _tpl=`<div class="login_panel" isLogin="">
		  <div class="panel_center">
		    <p class="tips">Canaan Creative</p>
		    <p class="connectBox1">
			<a title="EHash" href="https://ehash.com" target="_blank" rel="nofollow"><img alt="EHash" src="images/ehash.png"></a>
			<a title="Canaan Creative" href="http://www.canaan-creative.com" target="_blank" rel="nofollow"><img alt="Canaan Creative" src="images/home.png"></a>
			<a title="#Avalon" href="http://webchat.freenode.net/?randomnick=1&channels=avalon" target="_blank" rel="nofollow"><img alt="#avalon" src="images/irc.png"></a>
			<a title="service@canaan-creative.com" href="mailto:service@canaan-creative.com" target="_blank" rel="nofollow"><img alt="service@canaan-creative.com" src="images/email.png"></a>

			 </p>
		  </div>
		</div>`;
	return _tpl;
}
var loopNano = function() {
	for(var i of nanoObj)
		deviceTr(i);
}
var bindSettingBtn = function () {
	$("#setting-table").delegate("button" , "click" , function(){
		settingDialog({title:"Configuration for Avalon4 mini"} , paramObj);
		bindSettingSave();
	});
}

var bindSettingSave = function () {
	$("#setting-save-div").delegate("button" , "click" , function(){
		var _volt = parseInt($("#voltage-input").val());
		var _freq1 = parseInt($("#frequency1-input").val());	
		var _freq2 = parseInt($("#frequency2-input").val());	
		var _freq3 = parseInt($("#frequency3-input").val());	
		_volt = (window.isNaN(_volt) || _volt > 8000) ? 6500 : _volt;		
		_freq1 = (window.isNaN(_freq1) || _freq1 > 400) ? 100 : _freq1;
		_freq2 = (window.isNaN(_freq2) || _freq2 > _freq1) ? _freq1 : _freq2;
		_freq3 = (window.isNaN(_freq3) || _freq3 > _freq2) ? _freq2 : _freq3;
		var _param = {freqSet:[_freq1,_freq2,_freq3],voltSet:_volt};
		paramObj = _param;
		chrome.runtime.sendMessage({type: "setting", param:_param});
		$('#dialogModel').modal('hide');
		updateSetting();
	});
}
var bindPoolAdd = function () {
	$(".row-pool").delegate("button","click",function(){
		dialog({title:'Pool Setting'}, poolObj);
		bindSaveButton();				
	});
}

var bindSaveButton = function(callback) {
	$("#setting-pool").delegate("button" , "click" , function(){
		var maxLen = 3;
		var _pool = [];
		for( var i = 0; i < maxLen ; i++){
			var _address = _worker = _apikey = '';
			_address = $("#pool_address_"+i).val();
			_worker = $("#pool_worker_"+i).val();
			_apikey = $("#pool_apikey_"+i).val();
			if(!_address && !_worker && !_apikey)
				continue;
			if(_address.indexOf("://") >=0){
				var Pool_f = _address.split("://");
				_address = Pool_f[1];
			}
			var Pool_before = _address.split(":");
			_address = Pool_before[0];
			var _port = parseInt(Pool_before[1] || 3333);
			_pool.push({address:_address,port:_port,username:_worker,apiKey:_apikey});
		}	
		poolObj = _pool;
		chrome.runtime.sendMessage({type: "setting", pool:_pool});
		updatePoolList();	
		$('#dialogModel').modal('hide');
		if (callback !== undefined) {
			callback();
		}
	});
}


var bindPoolButton = function() {
	$("td").delegate("button","click",function(){
		switch( $(this).data('type') ) {
			case 'edit':
				editPool($(this).data('id'));
				break;
			case 'remove':
				removePool( $(this).data('id') );
				break;
		}
	});
}
var removePool = function(poolId) {
	chrome.runtime.sendMessage({info: "DeletePool", data:{poolId:poolId}});
	$("#pool-tr-id-"+poolId).remove();
	if($('#pool_info tr').size()===1)
		$("#pool_info").append('<tr id="pool-null"><td colspan="4" align="center" style="color:#cfcfcf;">Setting</td></tr>');
	 
}
var removeNano = function(nanoId) {
	$("#nano-tr-id-"+nanoId).remove();
	var _temp = [];
	for(var i of nanoObj){
		if(i.nanoId === nanoId)	
			continue;
		_temp.push(i);
	}
	nanoObj = _temp;
	if(!enterFlag)
		detectDevice();
	if($('#device tr').size()===1)
		$("#device").append('<tr id="device-null"><td colspan="5" align="center" style="color:#cfcfcf;">Insert usb device</td></tr>');
}


var maxPoolId = function() {
	return $("#pool_info tr:last-child").data('id');
}
var topPart = function() {
	var tpl = `<div class="row top-div">
		<ul>
			<li><p>BTC Price (USD/CNY): <span id="_huobi">0</span></p></li>
			<li><p>Difficulty: <span id="_difficulty">0</span></p></li>
			<li><p>Next Diff: <span id="_next">0</span></p></li>
			<li><p>Hash Rate: <span id="_hash_rate">0</span></p></li>
		</ul>
		<!--<p><button class="button button-small">BTC Price : <span id="Price">------<span></button></p>-->
        </div>
	`;
	return tpl;
}
var chartPart = function() {
	var tpl = `<div id="container" style="min-width:300px;height:300px"></div>`;
	return tpl;
} 
var panelPart = function(type , position , title ) {
	
	var partTpl = type === 'device' ? devicePart() : (type ==='pool' ? poolPart() : '');
	var col = type === 'device' ? 7 : 5;
	tpl = `<div class="col-xs-${col} col-md-${col} pull-${position}">
		<div class="panel panel-default">
		  <div class="panel-heading">
		    <h3 class="panel-title">${title}</h3>
		  </div>
		  <div class="panel-body" id="${type}_list">
		  ${partTpl}
		  </div>
		</div>
	</div>`;
	return tpl;
}
var settingPart = function() {
		var _tpl = `
		<div class="row col-xs-12">
			<div class="panel panel-default">
		  		<div class="panel-heading">
		    			<h3 class="panel-title">Configuration for Avalon4 mini</h3>
		  		</div>
		  		<div class="panel-body" id="setting-table">
					<table class="table" style="margin-bottom:0px;">
						<tr>
							<th>Voltage</th>
							<th>Frequency1</th>
							<th>Frequency2</th>
							<th>Frequency3</th>
							<th></th>
						</tr>
						<tr>
							<td id="voltage" >12</td>
							<td id="frequency1">22</td>
							<td id="frequency2">22</td>
							<td id="frequency3">22</td>
							<td>
								<button type="button" class="btn btn-default">Setting</button>
							</td>
						</tr>
					</table>
				</div>
		  	</div>
		</div>
			`;
	return _tpl;
}
var updatePoolList = function() {
	_tpl = poolPart();	
	$("#pool_list").html(_tpl);
	bindPoolAdd();
}
var updatePoolStatus = function ( poolInfo ) {
	//Object {info: "Active", type: "pool", poolId: 0}
	_tpl = "<p class='pool-status-" + poolInfo.info + "'>" + poolInfo.info + "</p>";
	$("#pool-status-"+poolInfo.poolId).html(_tpl);	
}
var updateDeviceStatus = function ( deviceInfo ) {
	if(deviceInfo.temperatureCu !== undefined)
		updateDeviceTemp(deviceInfo.deviceId , deviceInfo.temperatureCu , 'cu');
	if(deviceInfo.temperatureFan !== undefined)
		updateDeviceTemp(deviceInfo.deviceId , deviceInfo.temperatureFan , 'fan');
	if(deviceInfo.temperature !== undefined)
		updateDeviceTemp(deviceInfo.deviceId , deviceInfo.temperature , 'all');
	if(deviceInfo.version !== undefined)
		updateDeviceVersion(deviceInfo.deviceId , deviceInfo.version);
}
var updateDeviceVersion = function ( deviceId , version ) {
	$("#nano-version-" + deviceId).html( version );
}
var updateErrorRate = function ( deviceId , data ) {
	$("#nano-error-rate-" + deviceId).html( data );
}
var updateDeviceGHs5s = function ( deviceId , data ) {
	$("#nano-ghs5s-" + deviceId).html( data );
}
var updateDeviceTemp = function ( deviceId , temp , type ) {
	$("#" + type + "-temp-" + deviceId).html( temp );
}
var devicePart = function( data ) {
	var _tpl = '<table class="table table-hover" id="device">';
	_tpl += '<tr>';
	_tpl += '<th>Device</th>';
	_tpl += '<th>ID</th>';
	_tpl += '<th>Version</th>';
	_tpl += '<th>Temp(&deg;C)</th>';
	_tpl += '<th>GHs5s</th>';
	_tpl += '<th>Error%</th>';
	_tpl += '</tr>'; 
	_tpl += '</table>';
	return _tpl;
}
var poolPart = function( ) {
	var _tpl = '<table class="table table-hover" id="pool_info">';
	_tpl += '<tr>';
	_tpl += '<th>Address</th>';
	_tpl += '<th>Worker</th>';
	_tpl += '<th>Status</th>';
	_tpl += '</tr>'; 

	if(poolObj.length){
		for (var id in poolObj){
			if (poolObj[id] !== undefined && poolObj[id] !== null)
				_tpl +=poolTr(id , poolObj[id]);	
			
		}
	}else{
		_tpl +='<tr id="pool-null"><td colspan="3" align="center" style="color:#cfcfcf;">Setting</td></tr>';
	}
	_tpl += '</table>';
	_tpl += '<div class="row row-pool"><button type="button" class="btn btn-default pull-right pool-add">Setting</button></div>';
	return _tpl;
}

var poolTr = function(id , data) {
	var _tpl = '';	
	_tpl += '<tr data-id="'+ id +'" id="pool-tr-id-' + id + '" class="pool-tr-line">';
	_tpl += '<td id="pool-address-'+ id +'">' + data.address + ':' + data.port + '</td>';
	_tpl += '<td id="pool-worker-'+ id +'">' + data.username + '</td>';
	_tpl += '<td id="pool-status-' + id + '">---</td>';
	_tpl += '<input type="hidden" id="pool-api-key-'+id+'" value="'+data.apiKey+'"/>';
	_tpl += '</tr>';
	return _tpl;
}
var deviceTr = function(deviceObj) {
	if($("#device-null").html()!==undefined)
		$("#device-null").remove();
	var _tpl = '';
	var nanoId = deviceObj.nanoId;
	var deviceType = deviceObj.deviceType; 
	_tpl += '<tr class="active" id="nano-tr-id-' + nanoId + '">';
	_tpl += '<td>'+ deviceType +'</td>';
	_tpl += '<td id="nano-device-id-' + nanoId + '">' + nanoId + '</td>';
	_tpl += '<td id="nano-version-' + nanoId + '">---</td>';
	if (deviceType === 'Avalon nano')
		_tpl += '<td id="nano-temp-' + nanoId + '"><span id="all-temp-' + nanoId + '">0</span></td>';
	else
		_tpl += '<td id="nano-temp-' + nanoId + '"><span id="cu-temp-' + nanoId + '">0</span> / <span id="fan-temp-' + nanoId + '">0</span></td>';
	_tpl += '<td id="nano-ghs5s-'+ nanoId +'">0</td>';
	_tpl += '<td id="nano-error-rate-'+ nanoId +'">0</td>';
	_tpl += '</tr>';
	$("#device").append(_tpl);
}
var tablePart = function() {
	var tpl = '<div class="row" style="margin-bottom:50px;">';	
	tpl += panelPart('device' , 'left' , 'Device List'); 
	tpl += panelPart('pool' , 'right' , 'Pool List');
	tpl += '</div>';
	return tpl;
}

var settingPool = function() {
	dialog({title:'Setting pool'});
	bindSaveButton(mainPage);

}
var updateNanoData = function( nanoId , type , message) {
	switch(type){
		case 'status':
			if( message !== true ){
				$("#nano-tr-id-"+nanoId).removeClass("active").addClass("warning");
				$("#nano-status-"+nanoId).html('Connect failt');
			}else{
				 $("#nano-status-"+nanoId).html('Connect success');
			}
			break;
		case 'frequency':
			$("#nano-frequency-"+nanoId).html(message);
			break;
	}
}

var dialog = function( obj  , data ) {
	if ($("#dialogModel").length > 0){
		$("#dialogModel").remove();
		$(".modal-backdrop").remove();
	}
	var neo = {};
	neo.title = !!obj&&!!obj.title ? obj.title : 'Message';
	neo.close = !!obj&&!!obj.close ? obj.close : 'Close';
	neo.fade = !!obj&&!!obj.fade ? 'fade' : ''; /*Show speed*/

	var _tpl  = '<div>';
	_tpl += '<div class="form-group">';
	_tpl += '<table class="table">';
	_tpl += '<tr><th>Pool</th><th>Url</th><th>Worker</th><th>apiKey</th></tr>';
	for(var i= 0 ; i < 3 ; i ++){
		var _address = _worker = _apiKey = _port = '';

		if(data !== undefined){
			if(data[i] !==undefined ){
				_address = data[i].address || '';
				_worker = data[i].username || '';
				_apiKey = data[i].apiKey || '';
				_port = data[i].port || '';
				_address = _address + ':' + _port;
			}
		}
		_tpl +=	'<tr>';
		_tpl += '<td><label for="">#' + (i+1) + '</label></td>';
		_tpl += '<td><input type="text" class="form-control" value="' + _address + '" id="pool_address_' + i + '" placeholder="Pool url"></td>';	
		_tpl += '<td><input type="text" class="form-control" value="' + _worker + '" id="pool_worker_' + i + '" placeholder="Pool worker"></td>	';
		_tpl += '<td><input type="text" class="form-control" value="' + _apiKey + '" id="pool_apikey_' + i + '" placeholder="apiKey"></td>';	
		_tpl += '</tr>';
	}
	_tpl += '<tr><td colspan="4">';
	_tpl += '<div class="form-group" id="setting-pool">';
	_tpl += '<button typ="button" class="btn btn-default pull-right" id="add-pool-save">Save</button>';
	_tpl += '<span id="message"></span>';
	_tpl += '</div></td></tr></table></div></div>';
	neo.content = _tpl;

	neo.html = '<div class="modal '+neo.fade+'" id="dialogModel"><div class="modal-dialog modal-lg"><div class="modal-content">';
	neo.html += '<div class="modal-header"><button type="button" class="close" data-dismiss="modal" aria-hidden="true">&times;</button>';
	neo.html += '<h4 class="modal-title"><strong>' + neo.title + '</strong></h4></div>';
	neo.html += '<div class="modal-body">' + neo.content + '</div>';
	neo.html += '<div class="modal-footer"></div>';
	neo.html += '</div></div></div>';
	$("body").append(neo.html);
	$('#dialogModel').modal({
	      backdrop: 'static',
	      keyboard: false
	})
}
var updateSetting = function () {
	var data = paramObj;
	var _volt= data.voltSet;
	var _freq1 = data.freqSet[0]; 
	var _freq2 = data.freqSet[1]; 
	var _freq3 = data.freqSet[2]; 
	$("#voltage").html(_volt);	
	$("#frequency1").html(_freq1);	
	$("#frequency2").html(_freq2);	
	$("#frequency3").html(_freq3);	
}

var settingDialog = function( obj  , data ) {
	if ($("#dialogModel").length > 0){
		$("#dialogModel").remove();
		$(".modal-backdrop").remove();
	}
	var neo = {};
	neo.title = !!obj&&!!obj.title ? obj.title : 'Message';
	neo.close = !!obj&&!!obj.close ? obj.close : 'Close';
	neo.fade = !!obj&&!!obj.fade ? 'fade' : ''; /*Show speed*/
	
	var _volt= data.voltSet;
	var _freq1 = data.freqSet[0]; 
	var _freq2 = data.freqSet[1]; 
	var _freq3 = data.freqSet[2]; 
	
	var _tpl = `
		<div style="margin-bottom:50px;">
			<div class="form-group">
			<label for="Voltage">Voltage</label>
				<input type="text" class="form-control" id="voltage-input" value="${_volt}"placeholder="Voltage">
			</div>
			<div class="form-group">
			<label for="Frequency1">Frequency#1</label>
				<input type="text" class="form-control" id="frequency1-input" value="${_freq1}" placeholder="Frequency1">
			</div>
			<div class="form-group">
			<label for="Frequency2">Frequency#2</label>
				<input type="text" class="form-control" id="frequency2-input" value="${_freq2}" placeholder="Frequency2">
			</div>
			<div class="form-group">
			<label for="Frequency3">Frequency#3</label>
				<input type="text" class="form-control" id="frequency3-input" value="${_freq3}" placeholder="Frequency3">
			</div>
			<div class="form-group" id="setting-save-div">
			<button type="button" class="btn btn-default pull-right">Save</button>
			</div>
		</div>
	`;
	neo.content = _tpl;
	neo.html = '<div class="modal '+neo.fade+'" id="dialogModel"><div class="modal-dialog modal-lg"><div class="modal-content">';
	neo.html += '<div class="modal-header"><button type="button" class="close" data-dismiss="modal" aria-hidden="true">&times;</button>';
	neo.html += '<h4 class="modal-title"><strong>' + neo.title + '</strong></h4></div>';
	neo.html += '<div class="modal-body">' + neo.content + '</div>';
	neo.html += '<div class="modal-footer"></div>';
	neo.html += '</div></div></div>';
	$("body").append(neo.html);
	$('#dialogModel').modal({
	      backdrop: 'static',
	      keyboard: false
	})
}



$(function () {
	// render Highchart
	guidePage(guideCallback);	
	// notify background that the page is ready

	chrome.runtime.onMessage.addListener(function(msg, sender) {
		if (sender.url.indexOf('background') > -1)
			switch (msg.type) {
				case "setting":
					globalSetting(msg.param);
					poolList(msg.pool);
					break;
				case "device":
					nanoList({nanoId:msg.deviceId,deviceType:msg.deviceType});
					break;
				case "delete":
					removeNano(msg.deviceId);
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
