init_language = function(){
    var t = GetCookieByKey("languages");
    if (parseInt(t) >= 0) {
       init_call(t);
    }
}

$(function(){
    init_port();
    showweb();
    init_size();
    init_language();
    init_click();
    _init_load();
});

init_size = function(){
    var mtop = ($(window).height()-$(".log_div").height())/2;
    $("#centerDiv").css("margin-top",mtop);
}

_init_load = function(){
      StreamCfg();
}

init_click = function(){
    $("#loginuserName").focus();
    $("#log_login").click(function(){
        Check();
    });
    $("#log_reset").click(function(){
        $("#loginForm")[0].reset();
        $("#loginrtsp").val(port);
    });
    //点击语言选择框
    $(".languageshow").bind({
        click:function (e) {
            e.stopPropagation();
            $("#divLanguageChoose").toggle();
        }
    });
    //点击语言选择框以外的地方
    $("body").bind({
        click:function (e) {
            if ($("#divLanguageChoose").css("display") !== "none") {
                $('#divLanguageChoose').hide();
            }
        }
    });
}

init_call = function(t){
    $("#log_user").html(IDC_USER);
    $("#log_pwd").html(IDC_PWD);
    $("#log_rtsp").html(IDC_RTSPPORT);
    $("#ocx").html(IDC_DOWNLOAD);

    window.document.title = IDC_SUBMIT;

    $("#log_login").html(IDC_SUBMIT);
    $("#log_reset").html(IDC_RESET);
    
    $("#laCurrentLanguage").html(g_lan_arr[parseInt(t)]);

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

Language = function(type){
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

function Check(){
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
                window.location.href = "/asp/mainview.asp";
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
            window.location.href = "/asp/mainview.asp";
        }
    }catch(e){}
}

//Enter键
$(document).keypress(function(E){
    if (E.which == 13)
    {
        Check();
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
            Set_cookie("hdetect",jcpGet.hdetect);//人形侦测使能
            Set_cookie("cartect",jcpGet.cartect);//车辆侦测使能
            Set_cookie("facesnap",jcpGet.facesnap);//人脸抓拍显示使能
            Set_cookie("4g",jcpGet.__4g);//4g
            Set_cookie("wifi",jcpGet.wifi);//wifi

            if (jcpGet.webdeflang == 1) {
                document.getElementById('lan').style.display = 'none';
                g_lan_arr.shift();
                g_lan_js_arr.shift();
            } else {
                document.getElementById('lan').style.display = 'block';
                if (jcpGet.webdeflang == 2) {
                    g_lan_arr[0] = "русский";
                       g_lan_js_arr[0] = "russian.js";
                    $("#laCurrentLanguage").text("русский");
                    $($("#divLanguageChoose").find("div")[0]).find("label").text("русский");
                }
            }
            Set_cookie("webdeflang", jcpGet.webdeflang);
            var t = GetCookieByKey("languages");
            if (parseInt(t) < 0) {
                Language(0);
            }
        }else{
            showweb();
        }
    }});
}
