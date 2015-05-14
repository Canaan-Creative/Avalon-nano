var hashrate = [0, 0, 0];
var enterFlag = false;
var poolObj = [];
var nanoObj = [];
var renderChart = function() {
	Highcharts.setOptions({
		global: {useUTC: false}
	});
	$('#container').highcharts({
		chart: {
			type: 'spline',
			animation: Highcharts.svg, // don't animate in old IE
			marginRight: 10,
			events: {
				load: function() {
					var series = this.series;
					setInterval(function() {
						var x = (new Date()).getTime();
						for (var i in series)
							series[i].addPoint([x, hashrate[i]], true, true);
					}, 1000);
				}
			}
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
					s += '<tr><td><b>' + this.points[i].series.name + '</b>:&nbsp;</td><td>'
						+ numberShorten(this.points[i].y) + 'Hs</td></tr>';
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
			var names = ['5-Secondly', 'Minutely', 'Hourly'];
			var colors = ['#808080', '#feae1b', '#7B72E9'];
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
};
/*
setInterval(function(){
        $.ajax({
                url : 'https://data.btcchina.com/data/ticker?maket=all',
                success : function(msg){
                        $("#btc-price").html(msg.ticker.last);
                }
        });
}, 1000 * 2);
*/

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
	hashrate = [h.hs5s, h.hs1m, h.hs1h];
};
var nanoList = function(nanoId) {
	nanoObj.push(nanoId);
	console.log(nanoObj);
	if(enterFlag){
		deviceTr(nanoId);
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
			if(nanoObj.length === 1 ){
				detectFlag = true;
				$(".detect p img").remove();	
				var btnTpl = '<div style="text-align:center;"><button type="button" data-type="enter" class="btn btn-default"> Enter </button>  ';
				if(!poolObj.length)
					btnTpl +='  <button type="button" data-type="setting-pool" class="btn btn-default"> Setting pool </button>';
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

var loadingImg = "<img id='loadImg' src='images/loading.gif'/>";
var guidePage = function( ){
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
				<h4>To begin,plug your Ledger Wallet into the USB port of the computer</h4>
				<h5>if your Ledger Wallet is not recognized visit our help center</h5>
			</div>
			<p>Avalon nano chrome 12.01</p>
		</div>`;
		_mainObj.append(guidTpl);
		setTimeout(function(){
			$(".detect p img").attr("src","images/device.png");                
		},500);
		
	},500);
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
		_tpl += tablePart(); 
		_mainObj.html(_tpl);
		renderChart();
		bindPoolAdd();
		bindPoolButton();
		loopNano();
	},500);
}
var loopNano = function() {
	for(var i of nanoObj)
		deviceTr(i);
}
var bindPoolAdd = function () {
	$(".row-pool").delegate("button","click",function(){
		dialog({title:'Setting pool'});
		bindSaveButton();				
	});
}
var bindSaveButton = function(callback) {
	$(".form-group").delegate("button" , "click" , function(){
		var Pool_address = $("#address").val();
		var Pool_worker = $("#worker").val();
		if(!Pool_address.length || !Pool_worker){
			$("#message").html('Pool address or Worker is null').css("color","red");
			return;
		}
		if(Pool_address.indexOf("://") >=0){
			Pool_f = Pool_address.split("://");
			Pool_address = Pool_f[1];
		}
		var Pool_before = Pool_address.split(":");
		var Pool_url = Pool_before[0];
		var Pool_port = parseInt(Pool_before[1] || 3333);
		var t = parseInt($("#poolId").val());
		t = (t === -1) ? (maxPoolId() !== undefined ? maxPoolId() + 1 : 0) : t;
		chrome.runtime.sendMessage({info: "NewPool", data:{url:Pool_url,port:Pool_port,username:Pool_worker,poolId:t}});
		if($("#poolId").val()=="-1"){
			appendPool( t ,{url:Pool_url,port:Pool_port,username:Pool_worker});
		}else{
			changePool(t , Pool_url+':'+Pool_port , Pool_worker);
		}
		$('#dialogModel').modal('hide');
		if (callback !== undefined) {
			poolObj.push({url:Pool_url,port:Pool_port,username:Pool_worker});
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
		$("#pool_info").append('<tr id="pool-null"><td colspan="4" align="center" style="color:#cfcfcf;">Setting pool</td></tr>');
	 
}
var removeNano = function(nanoId) {
	$("#nano-tr-id-"+nanoId).remove();
	var _temp = [];
	for(var i of nanoObj){
		if(i === nanoId)	
			continue;
		_temp.push(i);
	}
	nanoObj = _temp;
	if(!enterFlag)
		detectDevice();
	if($('#device tr').size()===1)
		$("#device").append('<tr id="device-null"><td colspan="4" align="center" style="color:#cfcfcf;">Insert usb device</td></tr>');
}

var editPool = function(poolId) {
	var _editObj = {
		"address": $("#pool-address-"+poolId).html(),
		"worker" : $("#pool-worker-"+poolId).html(),
		"poolId" : poolId
	}
	dialog({title:'Update pool'} , _editObj);
	bindSaveButton();
}
var changePool = function(id , address , worker) {
	$("#pool-address-" + id).html(address);
	$("#pool-worker-" + id).html(worker);
}

var appendPool =  function(id , data){
	if($("#pool-null").html()!==undefined)
		$("#pool-null").remove();
	$("#pool_info").append(poolTr(id, data));
	bindPoolButton();
}
var maxPoolId = function() {
	return $("#pool_info tr:last-child").data('id');
}
var topPart = function() {
	var tpl = `<div class="row top-div">
		<p>btcPrice : 1459.23</p>
        </div>`;
	return tpl;
}
var chartPart = function() {
	var tpl = `<div id="container" style="min-width:300px;height:300px"></div>`;
	return tpl;
} 
var panelPart = function(type , position , title ) {
	
	var partTpl = type === 'device' ? devicePart() : (type ==='pool' ? poolPart() : '');
	tpl = `<div class="col-xs-6 col-md-6 pull-${position}">
		<div class="panel panel-default">
		  <div class="panel-heading">
		    <h3 class="panel-title">${title}</h3>
		  </div>
		  <div class="panel-body">
		  ${partTpl}
		  </div>
		</div>
	</div>`;
	return tpl;
}
var devicePart = function( data ) {
	var _tpl = '<table class="table table-hover" id="device">';
	_tpl += '<tr>';
	_tpl += '<th>Device</th>';
	_tpl += '<th>Temp</th>';
	_tpl += '<th>Frequency</th>';
	_tpl += '<th>Status</th>';
	_tpl += '</tr>'; 
	_tpl += '</table>';
	return _tpl;
}
var poolPart = function( ) {
	var _tpl = '<table class="table table-hover" id="pool_info">';
	_tpl += '<tr>';
	_tpl += '<th>Pool address</th>';
	_tpl += '<th>Worker</th>';
	_tpl += '<th>Status</th>';
	_tpl += '<th>Operation</th>';
	_tpl += '</tr>'; 

	if(poolObj.length){
		for (var id in poolObj){
			if (poolObj[id] !== undefined && poolObj[id] !== null)
				_tpl +=poolTr(id , poolObj[id]);	
			
		}
	}else{
		_tpl +='<tr id="pool-null"><td colspan="4" align="center" style="color:#cfcfcf;">Setting pool</td></tr>';
	}
	_tpl += '</table>';
	_tpl += '<div class="row row-pool"><button type="button" class="btn btn-default pull-right pool-add">Add pool</button></div>';
	return _tpl;
}

var poolTr = function(id , data) {
	var _tpl = '';	
	_tpl += '<tr data-id="'+ id +'" id="pool-tr-id-' + id + '" class="pool-tr-line">';
	_tpl += '<td id="pool-address-'+ id +'">' + data.url + ':' + data.port + '</td>';
	_tpl += '<td id="pool-worker-'+ id +'">' + data.username + '</td>';
	_tpl += '<td>Normal</td>';
	_tpl += '<td class="op">';
	_tpl += ' <button class="btn btn-default btn-xs" data-id="' + id + '" data-type="edit">Edit</button>';
	_tpl += ' <button class="btn btn-default btn-xs" data-id="' + id + '" data-type="remove">Remove</button>';
	_tpl += '</td>';
	_tpl += '</tr>';
	return _tpl;
}
var deviceTr = function(nanoId) {
	if($("#device-null").html()!==undefined)
		$("#device-null").remove();
	var _tpl = '';
	_tpl += '<tr class="active" id="nano-tr-id-' + nanoId + '">';
	_tpl += '<td id="nano-device-id-' + nanoId + '">nano' + nanoId + '</td>';
	_tpl += '<td>55</td>';
	_tpl += '<td id="nano-frequency-'+ nanoId +'">0</td>';
	_tpl += '<td id="nano-status-' + nanoId + '">Normal</td>';
	_tpl += '</tr>';
	$("#device").append(_tpl);
}
var tablePart = function() {
	var tpl = '<div class="row">';	
	tpl += panelPart('device' , 'left' , 'Device lists'); 
	tpl += panelPart('pool' , 'right' , 'Pool lists');
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

	var _address = !!data&&!!data.address ? data.address : '';
	var _worker = !!data&&!!data.worker ? data.worker : '';
	var _poolId = (data === undefined || data.poolId === undefined) ? '-1' : data.poolId;
	neo.content = `<div>
		<div class="form-group">
		<label for="Pool address">Pool address</label>
		<input type="text" class="form-control" id="address" placeholder="Pool address" value="${_address}">
		</div>
		<div class="form-group">
		<label for="Pool address">Worker</label>
		<input type="text" class="form-control" id="worker" placeholder="worker" value="${_worker}">
		</div>
		<div class="form-group">
		<input type="hidden" id="poolId" value="${_poolId}" />
		<button type="button" class="btn btn-default" id="add-pool-save"> Save </button>
		<span id="message"></span>
		</div>
	</div>`;
	neo.html = '<div class="modal '+neo.fade+'" id="dialogModel"><div class="modal-dialog"><div class="modal-content">';
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
	guidePage();	
	// notify background that the page is ready
	chrome.runtime.sendMessage({info: "Ready"});

	chrome.runtime.onMessage.addListener(function(msg, sender) {
		if (sender.url.indexOf('background') > -1)
			switch (msg.info) {
				case "PoolInit":
					console.log("PoolInit");
					poolList(msg.setting);
					break;
				case "NewNano":
					console.log("NewNano");
					nanoList(msg.nanoId);
					break;
				case "NanoDeleted":
					console.log("NanoDeleted");
					removeNano(msg.nanoId);
					break;
				case "NanoConnected":
					console.log("NanoConnected");
					updateNanoData(msg.nanoId,'status', msg.success);
					break;
				case "NanoDetected":
					console.log("NanoDetected");
					break;
				case "NewStatus":
					console.log("NewStatus");
					console.log(msg);
					updateNanoData(msg.nanoId,'frequency', msg.stat.frequency);
					break;
				case "PoolSubscribed":
					console.log("PoolSubscribed");
					break;
				case "PoolAuthorized":
					console.log("PoolAuthorized");
					break;
				case "Hashrate":
					updateHashrate(msg.hashrate);
					break;
			}
	});

});
