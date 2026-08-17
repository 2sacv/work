init_language = function(){
	var t = $.cookie("languages");
    init_call(t);
}

$(function(){
	init_port();
    showweb();
	init_language();
	init_click();
    _init_load();
    init_ocxversion();
});


init_ocxversion = function(){
    var g_has_ocx = true;
    if(g_is_msie){
      $("body").append('<object id="IPCamera" name="IPCamera" CLASSID="CLSID:2319F6E6-ABD3-4b68-BADF-05D8796FA072"></object>');
    }else{
      $("body").append('<object id="IPCamera" name="IPCamera" type="application/npipcam" width="0" height="0"></object>');
    }

    if(g_is_msie){
        var IPCamera = document.getElementById("IPCamera");
        if(IPCamera == null || IPCamera.object == null){
          g_has_ocx = false;
        }
    }else{
        var mimetype = navigator.mimeTypes["application/npipcam"];
        if(!(mimetype && mimetype.enabledPlugin)){
          g_has_ocx = false;
        }
    }
    if(g_has_ocx){
    	$("#ID_WEB_SERVICE").show();
    }

    $("#ID_DOWNLOAD_MANUAL").show();
    $("#ID_FOOTER").show();

    try 
    {
        //FireFox要求先失去焦点，才能得到焦点
        document.getElementById("loginuserName").blur();
        document.getElementById("loginuserName").focus();
    }
    catch (e) 
    {
    }
}

_init_load = function(){
      StreamCfg();
}

init_click = function(){
	$("#loginuserName").focus();
}

init_call = function(t){
	$("#log_user").html(IDC_USER);
	$("#log_pwd").html(IDC_PWD);
	$("#log_rtsp").html(IDC_RTSPPORT);

	$("#log_login").val(IDC_SUBMIT);
	$("#log_reset").val(IDC_RESET);
  
    $("#laCurrentLanguage").html(IDC_LANGUAGE);
    $("#ID_LANGUAGE").val(t);
    $("#ID_COPYRIGHT").html(IDC_VERSION);
    $("#ID_DOWNLOAD_COOMET_MANUAL").html(ID_DOWNLOAD_COOMET_MANUAL);
    $("#ID_DOWNLOAD_LINK_MANUAL").html(ID_DOWNLOAD_LINK_MANUAL);
    $("#ID_DOWNLOAD_LINK_MANUAL").attr("href", __target_link);

    GetJCP({cmd: "pelcod20ctrl -type 9 -cmd 17 -data2 "+t});
}

var port = "";
init_port = function(){
    var rtspport,upnpport;
    GetJCP({cmd: "portcfg -act list", ParseJCP: function(jcpobj){
           if(jcpobj!='Error'){
	           port = rtspport = jcpobj.rtsp;
	           upnpport = jcpobj.rtsp_upnp;
	           Set_cookie("webport",jcpobj.web);
		       Set_cookie("ftport",jcpobj.ftp);
			   if(document.URL.indexOf("3322")>=0){
					GetJCP({cmd: "ddns3322 -act list", ParseJCP: function(jcpobj){
						if(parseInt(jcpobj.ddnsen) == 1){
							GetJCP({cmd: "upnpcfg -act list", ParseJCP: function(upnp){
								if(parseInt(upnp.rtspen)==1){
								    port = upnpport;
									$("#loginrtsp").val(upnpport);
								}else{
	                               $("#loginrtsp").val(rtspport);
								}
							}});
						}else{
	                       $("#loginrtsp").val(rtspport);
						}
					}});
				}else{
					$("#loginrtsp").val(rtspport);
				} 
			}
	}});
}

Language = function(){
    var type = $("#ID_LANGUAGE").val();
    var t = $.cookie("languages");
    if(type != t){
		Set_cookie("languages",parseInt(type));
		var js = "/language/"+g_lan_js_arr[parseInt(type)];
		$.getScript(js).done(function(){
			init_call(type);
		}).fail(function(){
			Set_cookie("languages",t);
		});
	}
}

var mode=1;
var realm="";//数字验证

function OnClickLogin(){
    //判断是否启用用户名验证
    GetJCP({cmd: "authmode -act list", ParseJCP: function(jcpobj){
		try
		{
			mode = jcpobj.mode;
			if (1 == mode)
			{
				UserLogin();
			}
			else if (0 == mode)
			{
				var rtspport = $("#loginrtsp").val();
			    if (rtspport.length == 0)
			    {
			        alert(IDC_RTSPPORT_NOEMPTY);
			        window.focus();
			        return false;
			    }
			    
			    rtspport = parseInt(rtspport);
			    if (0 >= rtspport || 65536 <= rtspport)
			    {
			        alert(IDC_GEN_PORT_RANGE);
			        window.focus();
			        return false;
			    }
				Set_cookie("rtspport", rtspport);
				var strUrl = (document.URL.split('//')[1]).split('/')[0].split(':')[0];
			    Set_cookie("url", strUrl);
			    Set_cookie("loginflag_"+g_hostname, 1);
				window.location.href = "/asp/preview.asp";
			}
			else if (2 == mode){
				realm = jcpobj.realm;
				UserLogin();
			}
		}
        catch(E){}
    }});
}

function UserLogin(){

	var strName = $("#loginuserName").val();
	var strPasswd = $("#loginpasswd").val();
	var rtspport = $("#loginrtsp").val();

    if (strName.length == 0)
    {
        alert(IDC_USERNAME_NOEMPTY);
        $("#loginuserName").focus();
        return false;
    }
    if (strPasswd.length == 0)
    {
        alert(IDC_PASSWORD_NOEMPTY);
        $("#loginpasswd").focus();
        return false;
    }

    if (rtspport.length == 0)
    {
        alert(IDC_RTSPPORT_NOEMPTY);
        window.focus();
        return false;
    }
    
    rtspport = parseInt(rtspport);
    if (0 >= rtspport || 65536 <= rtspport)
    {
        alert(IDC_GEN_PORT_RANGE);
        window.focus();
        return false;
    }
	Set_cookie("rtspport", rtspport);

    if(mode == 2)
    {
    	strPasswd = $.md5(strName+":"+realm+":"+strPasswd);
    }
    
    var jcpcmd = "checkuser -act set -user "+$("#loginuserName").val()+" -password "+strPasswd;

    $.ajax({type: "GET", 
    	    url: "?jcpcmd=" + jcpcmd, 
    	    dataType: "script", 
    	    async: false,
    	    cache: false,
    	    success:function(result){
			    result = $.trim(result);
		        if (result.indexOf('[Success]')>0) {
		           ParseUserPasswdCfg(0);
		        }else{
		           var szResult = result.split("[Error]")[1];
		           ParseUserPasswdCfg(GetRtspKeyStr(szResult,"result"));
		        }
    	    },
    	    error:function(){
    	    	alert(IDC_LOGINFAIL);
    	    }
    });
}

function ParseUserPasswdCfg(result)
{   
	try
	{
		if(-3 == result){
			alert(IDC_ERR_USR);
			$("#loginuserName").focus();
			$("#loginuserName").val($("#loginuserName").val());
			return false;
		}
	     else if(-5 == result){
			alert(IDC_ERR_PWD);
			$("#loginpasswd").focus();
			$("#loginpasswd").val($("#loginpasswd").val());
	        return false;                    
		}
	    else if(0 <= result){
	    	var strUrl = (document.URL.split('//')[1]).split('/')[0].split(':')[0];
		    Set_cookie("url", strUrl);
		    Set_cookie("user", $("#loginuserName").val());
		    Set_cookie("passwd", $("#loginpasswd").val());
		    Set_cookie("loginflag_"+g_hostname, 1);
	        window.location.href = "/asp/preview.asp";
		}
	}catch(e){}
}

//Enter键
$(document).keypress(function(E){
	if (E.which == 13)
	{
		OnClickLogin();
	}
});

//页面窗口大小变化
$(window).resize(function(){
	var mtop = ($(window).height()-$(".log_div").height())/2;
	$("#centerDiv").css("margin-top",mtop);
});

//屏蔽非输入框下backspace引起的页面后退
$(document).keydown(function (e) {
    var varkey = e.keyCode || e.which || e.charCode; 
    if (varkey == 8 && e.target.tagName.toLowerCase()!='input') {
        e.preventDefault();
    }
});

function showweb(){
    GetJCP({cmd: "showweb -act list", ParseJCP: function(jcpGet){
        if(jcpGet !== "Error"){
            Set_cookie("has_dome",jcpGet.dome);
            Set_cookie("has_ptz_ctrl",jcpGet.ptz_ctrl);
            Set_cookie("has_3d",jcpGet.position_3D); //3d定位
            Set_cookie("has_MCUupgrade",jcpGet.MCUupgrade); //单片机升级
            Set_cookie("has_alarmin",jcpGet.alarmin); //报警输入
            Set_cookie("has_alarmout",jcpGet.alarmout); //报警输出
            Set_cookie("has_audio",jcpGet.audio);//音频设置
            Set_cookie("graintype",jcpGet.graintype);//产品类型
            Set_cookie("shelter",jcpGet.shelter);//球机隐私遮挡数目
        }else{
        	showweb();
        }
    }});
}

function OnMouseOutLogin() 
{
    document.getElementById("log_login").style.background = "url('/image/login/button_out.png')";
}

function OnMouseOverLogin() 
{
    document.getElementById("log_login").style.background = "url('/image/login/button_over.png')";
}

//按下重置按钮时调用
function OnClickReset()
 {
    document.getElementById("loginuserName").value = "";
    document.getElementById("loginpasswd").value = "";
    document.getElementById("loginrtsp").value = port;
}

function OnMouseOutReset() 
{
    document.getElementById("log_reset").style.background = "url('/image/login/button_out.png')";
}

function OnMouseOverReset() 
{
    document.getElementById("log_reset").style.background = "url('/image/login/button_over.png')";
}