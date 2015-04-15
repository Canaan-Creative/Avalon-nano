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

	console.log('=---------------------------------------------------=');

	chrome.storage.local.get('pool' , function(result){
		$("#pool_info_address_1").html(result.pool.address);
		$("#pool_info_worker_1").html(result.pool.worker);
	});
    });                                                                             

	$("#pool_info_edit_1").click(function() {
		console.log('Edit.......');	
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
	});
	
	$("#get_data").click(function(){
		console.log('Demo start ');
		var Pool_address = Pool_worker = '';
		chrome.storage.local.get('pool' , function(result) {
			console.log('pool Result address : ' + result.pool.address);
			console.log('pool Result worker : ' + result.pool.worker);
		});

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
	})
}); 


