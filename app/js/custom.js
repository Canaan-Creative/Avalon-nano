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
	for (var p of setting)
		if (p !== undefined)
			// add rows to pool table
			return;
};

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
					break;
				case "NanoDeleted":
					console.log("NanoDeleted");
					break;
				case "NanoConnected":
					console.log("NanoConnected");
					break;
				case "NanoDetected":
					console.log("NanoDetected");
					break;
				case "NewNonce":
					console.log("NewNonce");
					break;
				case "NewStatus":
					console.log("NewStatus");
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
		chrome.storage.local.get('pool' , function(result){
			if(typeof(result.pool) != "undefined")
				pool = result.pool;
			else
				pool = [];
			pool.push({'url' : Pool_address , 'worker' : Pool_worker});
			chrome.storage.local.set({'pool' : pool} , function() {
				console.log('Set pool success ' );
				console.log('Start loop pool');
				$('#myModal').modal('hide');
				$.setting._init();
			});
		});
	});
	$('#myModal').on('hidden.bs.modal', function (e) {
		$("#address").val('');
		$("#worker").val('');
		$("#message").html('');
		chrome.storage.local.get('pool' , function(result){
			//$("#pool_info_address_1").html(result.pool.address);
			//$("#pool_info_worker_1").html(result.pool.worker);
		});
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
				poolStr += '<tr> ';
				poolStr += '<td>'+ data[id].url+'</td>';
				poolStr += '<td>'+ data[id].worker+'</td>';
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
	}
}

