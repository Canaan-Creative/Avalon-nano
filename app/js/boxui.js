jQuery(document).ready
(
	function($) 
	{
		showLoginOnly(false);
		hide_cate();
		auto_show_adminBar();
	}
)

$(document).scroll(
	function() 
	{
		if($(".cate_form").css("display")=="block")
		{
			$(".cate_form").css("position","static");
			$("#container").css("margin-top","0");
		}
		minTop=$(document).scrollTop();
		
		if($(".cate_form").css("display")=="block")
			minTop-=72;
			
		if($("#wpadminbar").length>0)
			minTop-=28;
		
		/*正文右侧边栏自动悬浮广告*/
		if($("#auto_fload_ad"))
		{
			if(minTop>484)
			{
				$("#auto_fload_ad").css("position","fixed");
				
				if($("#wpadminbar").length>0)
					$("#auto_fload_ad").css("top","80px");
				else
					$("#auto_fload_ad").css("top","52px");
			}
			else
			{
				$("#auto_fload_ad").css("position","static");
			}
		}
	}
)
//同一IP 12小时只显示一次
function showLoginOnly(isClose)
{　
	var isLoginS=$('.login_panel').attr("isLogin");
	var cookieString = new String(document.cookie);
	var cookieHeader = 'boxui_com_login_panel='; //更换happy_pop为任意名称
	var beginPosition = cookieString.indexOf(cookieHeader);
	if(isClose)
	{
		if (isClose)
		{
			$(".login_panel").css("display","none");
			var refrushTime = new Date();　　　　
		　 　refrushTime.setTime(refrushTime.getTime() + 12*60*60*1000 ) //同一ip设置过期时间，即多长间隔跳出一次
		　  document.cookie = 'boxui_com_login_panel=yes;expires='+ refrushTime.toGMTString();　 //更换happy_pop和第4行一样的名称
		}
	}
	else if(isLoginS=="" && beginPosition<0)
		$(".login_panel").css("display","block");
}

function hide_cate()
{
	if($("#show_status").attr("className")=="show_status_down")
		$("#wrap").css("padding-top","55px");
	$("#show_status").click
	(
		function()
		{
			if($("#show_status").attr("className")=="show_status_up")
			{
				$(".cate_form").slideUp(300);
				$("#show_status").attr("className","show_status_down");
				$("#wrap").css("padding-top","55px");
			}
			else{
					$(".cate_form").slideDown(300);
					minTop=$(document).scrollTop();
					if(minTop>70) 
					{
						$(".cate_form").css("position","fixed");
					}
					else 
					{
						$(".cate_form").css("position","static");
						$("#wrap").css("padding-top","15px");
					}
					
						
					$("#show_status").attr("className","show_status_up");
			}
		}
	)
}

function auto_show_adminBar()
{
	/*Add by dream 2013-3-19  解决登录后顶部管理栏挡住网站头部*/
  var adminbar=$($("#wpadminbar"));
  if(adminbar && ($("#wpadminbar").css("display")=="block"))
  {
  	$("#header").css({top:28});
  }
  else
  {
	 $("#header").css({top:0});
  }	
}
