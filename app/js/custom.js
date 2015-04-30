var renderChart = function() {
	var MHz = 0;
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

					// set up the updating of the chart each second
					var series = this.series[0];
					setInterval(function() {
						var x = (new Date()).getTime(), // current time
						//y = Math.random();
						y = MHz;
						series.addPoint([x, y], true, true);
					}, 1000);
				}
			}
		},
		title: {text: 'Live random data'},
		xAxis: {
			type: 'datetime',
			tickPixelInterval: 150
		},
		yAxis: {
			title: {text: 'Value'},
			plotLines: [{value: 0, width: 1, color: '#808080'}]
		},
		tooltip: {
			formatter: function() {
				return '<b>' + this.series.name + '</b><br/>' +
					Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', this.x) + '<br/>' +
					Highcharts.numberFormat(this.y, 2);
			}
		},
		credits: {enabled: false},
		legend: {enabled: false},
		exporting: {enabled: false},
		series: [{
			name: 'Random data',
			data: (function() {
				// generate an array of random data
				var data = [],
				time = (new Date()).getTime(),
				i;

				for (i = -19; i <= 0; i++) {
					data.push({
						x: time + i * 1000,
						y: Math.random()
					});
				}
				return data;
			})()
		}]
	});
};

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
				case "NewNonce":
					console.log("NewNonce");
					break;
				case "NewStatus":
					console.log("NewStatus");
					$.setting._updateNano(msg.nanoId,'frequency', msg.stat.frequency);	
					break;
				case "PoolSubscribed":
					console.log("PoolSubscribed");
					break;
				case "PoolAuthorized":
					console.log("PoolAuthorized");
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
		Pool_address = Pool_address.indexOf(":") == -1 ? Pool_address+':3333' : Pool_address;
		var Pool_before = Pool_address.split(":");
		var Pool_url = Pool_before[0];
		var Pool_port = Pool_before[1];
		var t = $.setting._maxPoolId() != undefined ? $.setting._maxPoolId()+1 : 0 ;	
		chrome.runtime.sendMessage({info: "NewPool", data:{url:Pool_url,port:Pool_port,username:Pool_worker,poolId:t}});
		$('#myModal').modal('hide');
	});
	$('#myModal').on('hidden.bs.modal', function (e) {
		$("#address").val('');
		$("#worker").val('');
		$("#message").html('');
		console.log('Close modal');
	});

});

jQuery.setting = {
	_init : function () {
		chrome.storage.local.get( 'pool' , function( result ) {
			if(typeof(result.pool) != "undefined")
				$.setting._pool( result.pool );
			else
				$.setting._pool( result.pool);

			$("td").delegate("button","click",function(){
				var _type = $(this).data('type');
				var _objId = $(this).data('id');
				switch( _type ) {
					case 'edit':
						console.log('edit...................');
						break;
					case 'remove':
						$.setting._remove( _objId );
						break;
				}
			});
		});
	},

	_hide : function ( id ) {
		$(id).hide();
	},
	_show : function ( id ) {
		$(id).show();
	},

	_remove : function ( id  ) {
		chrome.storage.local.get( 'pool' , function( result ) {
			if(typeof(result.pool) != "undefined") {
				var length = result.pool.length;
				if(length < 2){
					chrome.storage.local.remove( 'pool' , function(){});
				}else{
					var newpool = [];
					for(var obj in result.pool){
						if(obj != id)
							newpool.push(result.pool[obj]);
					}
					chrome.storage.local.set({ 'pool' : newpool} , function() {});
				}
			}
			$.setting._init();
		});
	},
	_pool : function ( data ) {
		var poolStr = '';
		poolStr += '<tr>';
		poolStr += '<th width="25%">Pool address</th>';
		poolStr += '<th width="25%">worker</th>';
		poolStr += '<th width="25%">Status</th>';
		poolStr += '<th width="25%">Operation</th>';
		poolStr += '</tr>';
		if( data != undefined){
			for( var id  in data){
				poolStr += '<tr data-id="'+id+'"> ';
				poolStr += '<td>'+ data[id].url+':'+ data[id].port+'</td>';
				poolStr += '<td>'+ data[id].username+'</td>';
				poolStr += '<td>Normal</td>';
				poolStr += '<td class="op">';
				poolStr += ' <button class="button button-tiny button-highlight button-small" data-id="'+ id +'" data-type="edit">Edit</button>';
				poolStr += ' <button class="button button-tiny button-caution button-small" data-id="'+ id +'" data-type="remove">Remove</button>';
				poolStr += '</td>';
				poolStr += '</tr>';
			}
		} else {
			poolStr += '<tr><td colspan="4" align="center" style="color:red;">Setting pool</td></tr>';
		}
		$("#pool_info").html( poolStr );

		$("td").delegate("button","click",function(){
			switch( $(this).data('type') ) {
				case 'edit':
					console.log('edit...................');
					break;
				case 'remove':
					$.setting._removePool( $(this).data('id') );
					break;
			}
		});
	},
	_removePool : function(poolId) {
		console.log('removiePool  : ' + poolId);
		chrome.runtime.sendMessage({info: "DeletePool", data:{poolId:poolId}});
	},
	_editPool : function(poolId , data) {
	
	},
	_addNano : function (nanoId) {
		var nanoStr = '';	
		nanoStr += '<tr class="active" id="nano-tr-id-' + nanoId + '">';
		nanoStr += '<td id="nano-device-id-' + nanoId + '">nano' + nanoId + '</td>';   
		nanoStr += '<td>55</td>';      
		nanoStr += '<td id="nano-frequency-"'+ nanoId +'>0</td>';     
		nanoStr += '<td id="nano-status-' + nanoId + '">Normal</td>';   
		nanoStr += '<td id="nano-version-' + nanoId + '">12.01</td>';   
		nanoStr += '</tr>';
		$("#device").append(nanoStr);
	},	
	_updateNano : function (nanoId , type , message ,version){
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

