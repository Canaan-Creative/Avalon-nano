var hashrate = [0, 0, 0];
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
		title: {text: 'Live Hashrate'},
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
		legend: {enabled: false},
		exporting: {enabled: false},
		series: (function() {
			var series = [];
			var names = ['5-Secondly', 'Minutely', 'Hourly'];
			var colors = ['#4cb0f9', '#a49ef0', '#ff7680'];
			var data = [];
			var time = (new Date()).getTime();
			for (var i = -899; i <= 0; i++) {
				data.push({
					x: time + i * 1000,
					y: 0
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
	var totalHashrate = [0, 0, 0];
	for (var h of hashrates){
		totalHashrate[0] += h.hs5s;
		totalHashrate[1] += h.hs1m;
		totalHashrate[2] += h.hs1h;
	}
	hashrate = totalHashrate;
}
var poolInit = function(setting) {
	console.log(setting);
	$.setting._pool( setting );
	return;
	for (var p of setting)
		if (p !== undefined)
			// add rows to pool table
			return;
};

var nanoList = function(nanoId) {
	$.setting._addNano(nanoId);
}


$(function () {
	// render Highchart
	renderChart();

	// notify background that the page is ready
	chrome.runtime.sendMessage({info: "Ready"});

	chrome.runtime.onMessage.addListener(function(msg, sender) {
		if (sender.url.indexOf('background') > -1)
			switch (msg.info) {
				case "PoolInit":
					console.log("PoolInit");
					poolInit(msg.setting);
					break;
				case "NewNano":
					console.log("NewNano");
					nanoList(msg.nanoId);
					break;
				case "NanoDeleted":
					console.log("NanoDeleted");
					$.setting._removeNano(msg.nanoId);
					break;
				case "NanoConnected":
					console.log("NanoConnected");
					$.setting._updateNano(msg.nanoId,'status', msg.success);
					break;
				case "NanoDetected":
					console.log("NanoDetected");
					$.setting._updateNano(msg.nanoId,'version', msg.success, msg.version);
					break;
				case "NewStatus":
					console.log("NewStatus");
					console.log(msg);
					$.setting._updateNano(msg.nanoId,'frequency', msg.stat.frequency);
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

	$("#pool_info_edit_1").click(function() {
		var pool_address = pool_worker = '';
		chrome.storage.local.get('pool' , function(result){
			pool_address = result.pool.address;
			pool_worker= result.pool.worker;
			$("#address").val(pool_address);
			$("#worker").val(pool_worker);
		});
		$("#myModal").modal('show');
	});
	$("#pool_info_remove_1").click(function() {
		$.setting._remove('pool');
		$.setting._init();
	});
	$("#add-pool-save").click(function(){
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
		var t = $("#poolId").val() =="-1" ? ($.setting._maxPoolId() !== undefined ? $.setting._maxPoolId()+1 : 0) : $("#poolId").val();
		chrome.runtime.sendMessage({info: "NewPool", data:{url:Pool_url,port:Pool_port,username:Pool_worker,poolId:t}});
		if($("#poolId").val()=="-1"){
			$.setting._appendPool( t ,{url:Pool_url,port:Pool_port,username:Pool_worker});
		}else{
			$("#pool-address-"+t).html(Pool_url+':'+Pool_port);
			$("#pool-worker-"+t).html(Pool_worker);
		}
		$('#myModal').modal('hide');
	});
	$('#myModal').on('hidden.bs.modal', function (e) {
		$("#address,#worker").val('');
		$("#message").html('');
		$("#poolId").val("-1");
		console.log('Close modal');
	});

});

jQuery.setting = {
	_init : function () {
	},

	_hide : function ( id ) {
		$(id).hide();
	},
	_show : function ( id ) {
		$(id).show();
	},
	_pool : function ( data ) {
		var poolHandel = true;
		var poolStr = `<tr>
			<th width="25%">Pool address</th>
			<th width="25%">worker</th>
			<th width="25%">Status</th>
			<th width="25%">Operation</th>
			</tr>`;
		if( data !== undefined || data !==null){
			for (var id in data){
				if (data[id] !== undefined && data[id] !== null){
					poolStr += $.setting._poolHtml(id ,data[id]);
					poolHandel = false;
				}
			}
		}
		if(poolHandel){
			poolStr += '<tr id="pool-info-footer"><td colspan="4" align="center" style="color:red;">Setting pool</td></tr>';
		}
		$("#pool_info").html( poolStr );
		$.setting._bindButton();
	},
	_bindButton : function (){
		$("td").delegate("button","click",function(){
			switch( $(this).data('type') ) {
				case 'edit':
					$.setting._editPool($(this).data('id'));
					break;
				case 'remove':
					$.setting._removePool( $(this).data('id') );
					break;
			}
		});
	},
	_poolHtml : function (id , data) {
		var poolStr = '';
		poolStr += '<tr data-id="'+ id +'" id="pool-tr-id-' + id + '" class="pool-tr-line">';
		poolStr += '<td id="pool-address-'+ id +'">' + data.url + ':' + data.port + '</td>';
		poolStr += '<td id="pool-worker-'+ id +'">' + data.username + '</td>';
		poolStr += '<td>Normal</td>';
		poolStr += '<td class="op">';
		poolStr += ' <button class="button button-tiny button-highlight button-small" data-id="' + id + '" data-type="edit">Edit</button>';
		poolStr += ' <button class="button button-tiny button-caution button-small" data-id="' + id + '" data-type="remove">Remove</button>';
		poolStr += '</td>';
		poolStr += '</tr>';
		return poolStr;
	},
	_appendPool : function(id , data){
		if($("#pool-info-footer").html()!==undefined)
			$("#pool-info-footer").remove();
		$("#pool_info").append($.setting._poolHtml(id, data));
		$.setting._bindButton();
	},
	_removePool : function(poolId) {
		chrome.runtime.sendMessage({info: "DeletePool", data:{poolId:poolId}});
		$("#pool-tr-id-"+poolId).hide('slow' , function(){
			$("#pool-tr-id-"+poolId).remove();
		});
	},
	_editPool : function(poolId) {
		$("#address").val($("#pool-address-"+poolId).html());
		$("#worker").val($("#pool-worker-"+poolId).html());
		$("#poolId").val(poolId);
		$("#myModal").modal('show');
	},
	_addNano : function (nanoId) {
		var nanoStr = '';
		nanoStr += '<tr class="active" id="nano-tr-id-' + nanoId + '">';
		nanoStr += '<td id="nano-device-id-' + nanoId + '">nano' + nanoId + '</td>';
		nanoStr += '<td>55</td>';
		nanoStr += '<td id="nano-frequency-'+ nanoId +'">0</td>';
		nanoStr += '<td id="nano-status-' + nanoId + '">Normal</td>';
		nanoStr += '<td id="nano-version-' + nanoId + '">12.01</td>';
		nanoStr += '</tr>';
		$("#device").append(nanoStr);
	},
	_updateNano : function (nanoId , type , message ,version){
		console.log(nanoId + '----' + type +'----' + message );
		switch(type){
			case 'status':
				if( message != true ){
					$("#nano-tr-id-"+nanoId).removeClass("active").addClass("warning");
					$("#nano-status-"+nanoId).html('Connect failt');
				}else{
					 $("#nano-status-"+nanoId).html('Connect success');
				}
				break;
			case 'version':
				$("#nano-version-"+nanoId).html(version);
				if( message != true){
					$("#nano-tr-id-"+nanoId).removeClass("active").addClass("warning");
					$("#nano-status-"+nanoId).html('Detect failt');
				}else{
					$("#nano-status-"+nanoId).html('Detect pass');
				}
				break;
			case 'frequency':
				$("#nano-frequency-"+nanoId).html(message);
				break;
		}
	},
	_removeNano : function(nanoId) {
		$("#nano-tr-id-"+nanoId).remove();
	},
	_maxPoolId : function() {
		return $("#pool_info tr:last-child").data('id');
	}

}

