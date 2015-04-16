$(function () {                                                                     
    $(document).ready(function() {                                                  
        Highcharts.setOptions({                                                     
            global: {                                                               
                useUTC: false                                                       
            }                                                                       
        });                                                                         
                                                                                    
        var chart;                                                                  
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
                                y = Math.random();                                  
                            series.addPoint([x, y], true, true);                    
                        }, 1000);                                                   
                    }                                                               
                }                                                                   
            },                                                                      
            title: {                                                                
                text: 'Live random data'                                            
            },                                                                      
            xAxis: {                                                                
                type: 'datetime',                                                   
                tickPixelInterval: 150                                              
            },                                                                      
            yAxis: {                                                                
                title: {                                                            
                    text: 'Value'                                                   
                },                                                                  
                plotLines: [{                                                       
                    value: 0,                                                       
                    width: 1,                                                       
                    color: '#808080'                                                
                }]                                                                  
            },                                                                      
            tooltip: {                                                              
                formatter: function() {                                             
                        return '<b>'+ this.series.name +'</b><br/>'+                
                        Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', this.x) +'<br/>'+
                        Highcharts.numberFormat(this.y, 2);                         
                }                                                                   
            },                                                                      
            legend: {                                                               
                enabled: false                                                      
            },                                                                      
            exporting: {                                                            
                enabled: false                                                      
            },                                                                      
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


	$.setting._init();
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
		console.log('address :' + Pool_address + ' | worker : ' + Pool_worker);
		var pool = {};
		pool.address = Pool_address;
		pool.worker = Pool_worker;
		chrome.storage.local.set({'pool' : pool} , function() {
			console.log('Set pool success ~ ');	
		});
		$('#myModal').modal('hide');
		$.setting._init();
	});
	$('#myModal').on('hidden.bs.modal', function (e) {
		$("#address").val('');
		$("#worker").val('');
		$("#message").html('');
		chrome.storage.local.get('pool' , function(result){
			$("#pool_info_address_1").html(result.pool.address);
			$("#pool_info_worker_1").html(result.pool.worker);
		});
		console.log('Close modal');
	});
}); 

jQuery.setting = {

	_init : function () {
		console.log('init .........');
		var pool_address = pool_worker = '';
		chrome.storage.local.get('pool' , function(result){
			console.log(typeof(result.pool));
			if(typeof(result.pool) != "undefined"){
				$.setting._show("#pool_info_row_1");	
				pool_address = result.pool.address;
				pool_worker = result.pool.worker;
				$("#pool_info_address_1").html(result.pool.address);
				$("#pool_info_worker_1").html(result.pool.worker);
				$.setting._hide(".add_pool_button");
			}else{
				$.setting._hide("#pool_info_row_1");	
				$.setting._show(".add_pool_button");
			}

		});
	},
	_hide : function ( id ) {
		$(id).hide();			
	},
	_show : function ( id ) {
		$(id).show();	
	},
	_remove : function ( key ) {
		chrome.storage.local.remove( key , function(){
			console.log('Remove ' + key + 'success ~');
		});
	}

} 


