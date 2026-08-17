$(function(){
  var lf = $.cookie("loginflag_"+g_hostname);
  if (null === lf || typeof(lf) =='undefined' || lf === "null" || 0 > parseInt(lf)){
    location.href = "/login.asp";
  }else{
    if($.cookie("graintype") == 0){
      $("#spanPlayback").hide();
    }
  	_init_language();
  	_init_click();
    window.document.title = IDC_PARAMETER_SET;
    homeFrame = window.parent.document.getElementById("homeFrame");
    getPlatform();
    initMenuTree();
    init_auth();
    init_show_menu();
    resizePage();
    setTimeShow();
    $(window).resize(function(){
      resizePage();
    });
  }
});

function setTimeShow(){
  document.getElementById('spanCurrTime').innerHTML = date();
  setTimeout(function(){setTimeShow();},1000);
}

function resizePage(){
  var _h = $(".side").height();
  if(_h < 100){
    $("#menuTreeDiv").css("height", 100);
  }else{
    $("#menuTreeDiv").css("height", _h-54);
  }
  
  $("#homeFrame").css("width", $(window).width()- 275 - 2);
  $("#homeFrame").css("height", $(window).height() - 85 - 2);
  
}

_init_click = function(){
     $(".side span").click(function(){
          switch($(this)[0].id){
            case "spanExit":
              if(confirm(IDC_MSGBOX_MSG)){
                 $.cookie("lastclickmenu",null);
                 deleteCookie("loginflag_"+g_hostname);
                 location.href = "../login.asp"
              }
              break;
            case "spanPlayback":
              $.cookie("lastclickmenu",null);
              location.href = "playback.asp";
              break;
            case "spanSetting":
              break;
            case "spanLog":
              $.cookie("lastclickmenu",null);
              location.href = "log.asp";
              break;
            case "spanLiveview":
              $.cookie("lastclickmenu",null);
              location.href = "preview.asp";
              break;
            default:
              break;
          }
      });
 
  	
}

function _init_language(){
  $("#laLiveview").html(IDC_PLAYVIDEO);
  $("#laLog").html(IDC_LOG);
  $("#laSetting").html(IDC_PARAMETER_SET);
  $("#laPlayback").html(IDC_PLAYBACK);
  $("#laExit").html(IDC_EXIT);
}

var homeFrame;//页面Iframe
var defFirstMenuBgColor = "rgb(169,175,183)";
var selMenuBgColor = "rgb(49,106,197)";

//最近一次点击的菜单
var m_arrLastClickData = [];

//一级菜单
var m_arrFirstMenu = [ IDC_MENU_SYSTEM_SET,
                       IDC_MENU_NETWORKSET,
                       IDC_MENU_AUDIOVIDEO,
                       IDC_MENU_VIDEOCAPTURE,
                       IDC_ALARM_SET,
                       IDC_MENU_LOCALSET,
                       IDC_MENU_PLATFORMSET];

//二级菜单(菜单名称，是否显示，url)
var m_arrSecondMenu = [
                        //系统设置
                        [[IDC_BASICINFO,true,"basicinfo.asp"],
                        [IDC_TIMESETTING,true,"timesetting.asp"],
                        [IDC_SYSOPERA,true,"sysopt.asp"],
                        [IDC_SYSUPDATE,true,"sysupdate.asp"],
                        [IDC_USERMANAGEMENT,true,"usermanage.asp"],
                        [IDC_PORTSETTING,true,"serialport.asp"],
                        [IDC_SYSTEMSTATUS,true,"sysstatus.asp"],
                        [IDC_DMUPDATE,false,"dmupdate.asp"]],

                        //网络设置
                        [[IDC_ETHNET,true,"ethnet.asp"],
                        [IDC_PPPOE,true,"pppoe.asp"],
                        [IDC_DDNS,true,"ddns.asp"],
                        [IDC_NETPORT,true,"netport.asp"],
                        [IDC_EMAIL_SETTING,true,"emailsetting.asp"],
                        [IDC_FTPCLI_SETTING,false,"ftpsetting.asp"],
                        [IDC_NETCHECK,true,"netcheck.asp"]],

                        //音频视频
                        [[IDC_CHANNELSETTING,true,"videochannel.asp"],
                        [IDC_SUBTITLE_OVERLAY,true,"overlay.asp"],
                        [IDC_VIDEOSETTING,true,"imagesetting.asp"],
                        [IDC_MENU_PRIVACY_MASK,true,"privacysetting.asp"],
                        [IDC_ROI_SETTING,true,"roisetting.asp"],
                        [IDC_AUDIOSET,true,"audiosetting.asp"],
                        [IDC_FILL_LIGHT_SETTING,true,"infraredsetting.asp"],
                        [IDC_EXTEND_DISPOSE,true,"extendsetting.asp"]],

                        [[]],

                        //报警设置
                        [[IDC_MENU_MOTION,true,"motionalarm.asp"],
                        [IDC_monitor_cross_alarm,true,"vgline.asp"],
                        [IDC_monitor_area_alarm,true,"vgarea.asp"],
                        [IDC_HUMAN_DETECTION_ALARM,true,"humandetect.asp"],
                        [IDC_VL_TIME_STRATEGY,true,"vlalarm.asp"],
                        [IDC_VM_TIME_STRATEGY,true,"vmalarm.asp"],
                        [IDC_ALARMLINKATTR,false,"ioalarm.asp"],
                        [IDC_ALARMLINKAGE,true,"alarmlink.asp"],
                        [IDC_VOICE_LIGHT_LINK,true,"voicelightlink.asp"]],

                        //本地设置
                        [[IDC_MENU_LOCALSET,true,"localsetting.asp"]],

                        //云服务
                        [[IDC_DANALE_PLATFORM,false,"hbptp.asp"],
                        [IDC_GUOB,false,"guobiao.asp"]]
                    ];

function initMenuTree(){
    var _html = "";
    for(var i=0,l=m_arrFirstMenu.length;i<l;i++){
        _html += "<div id='menu"+(i+1)+"' class='firstclass' onclick='onLevelMenuClick(false,\"pop"+(i+1)+"\",\""+m_arrSecondMenu[i][0][2]+"\",\""+i+"\");'>";
        _html += "<div class='subfirstclass'>";
        _html += "<img style='float:left;margin-left:5px;' src='../image/menu_closed.png' id='imgpop"+(i+1)+"'>";
        _html += "<img style='float:left;margin-left:5px;' src='../image/menu_first.png'>";
        _html += "<span id='sub"+(i+1)+"Span'>"+m_arrFirstMenu[i]+"</span>";
        _html += "</div>";
        _html += "</div>";
        _html += "<div class='secondclass displaynone' id='pop"+(i+1)+"'>";
        
        for(var j=0,h=m_arrSecondMenu[i].length;j<h;j++){
            var _display = m_arrSecondMenu[i][j][1]==true?"block":"none";
            _html += "<div id='sub"+(i+1)+"_"+(j+1)+"' class='secondclassdiv' onclick='onLevelMenuClick(true,\"sub"+(i+1)+"_"+(j+1)+"\",\""+m_arrSecondMenu[i][j][2]+"\",\""+i+"\",\""+j+"\")' style='display:"+_display+"'>";
            _html += "<div class='subsecondclass'>";
            _html += "<img src='../image/menu_settings.png'>";
            _html += "<span id='sub"+(i+1)+"_"+(j+1)+"Span'>"+m_arrSecondMenu[i][j][0]+"</span>";
            _html += "</div>";
            _html += "</div>";
        }

        _html += "</div>";
    }
    $("#menuTreeDiv").html(_html);
}

function init_show_menu(){
    var lastClickMenu = $.cookie("lastclickmenu");
    if(lastClickMenu == null){
        m_arrLastClickData[0] = "pop1";
        m_arrLastClickData[1] = "sub1_1";
        m_arrLastClickData[2] = m_arrSecondMenu[0][0][2];
    }else{
        var menuArr = lastClickMenu.split(";");
        m_arrLastClickData[0] = menuArr[0];
        m_arrLastClickData[1] = menuArr[1];
        m_arrLastClickData[2] = menuArr[2];
    }
    document.getElementById(m_arrLastClickData[0]).style.display = "block";

    $("#"+m_arrLastClickData[1]).css("background",selMenuBgColor);

    $("#imgpop"+m_arrLastClickData[0].substring(3)).attr('src',"../image/menu_opened.png");

    homeFrame.src = m_arrLastClickData[2];
}


function onLevelMenuClick(isSub,szID,szUrl,iIndex,jIndex){
    if(!isSub && m_arrLastClickData[0] != "" && m_arrLastClickData[0] == szID){//重复点击当前展开的一级菜单
        return;
    }
    if(!!isSub && m_arrLastClickData[1] != "" && m_arrLastClickData[1] == szID){//重复点击当前显示的二级菜单
        return;
    }

    if(!isSub && m_arrLastClickData[0] != "" && m_arrLastClickData[0] != szID){//选择不同的一级菜单
       
        document.getElementById(m_arrLastClickData[0]).style.display = "none";//隐藏前一个展开的菜单

        $("#imgpop"+m_arrLastClickData[0].substring(3)).attr('src',"../image/menu_closed.png");
   
        document.getElementById(szID).style.display = "block";//显示选择的菜单

        document.getElementById(m_arrLastClickData[1]).style.background= defFirstMenuBgColor;

        m_arrLastClickData[1] = "sub"+szID.substring(3,szID.length)+"_1";
       
        m_arrLastClickData[0] = szID;
        
        document.getElementById(m_arrLastClickData[1]).style.background= selMenuBgColor;

        $("#imgpop"+m_arrLastClickData[0].substring(3)).attr('src',"../image/menu_opened.png");

        $.cookie("lastclickmenu",szID+";"+m_arrLastClickData[1]+";"+szUrl);
    }else{ //选择不同的二级菜单

        document.getElementById(m_arrLastClickData[1]).style.background= defFirstMenuBgColor;

        m_arrLastClickData[1] = szID;
        document.getElementById(szID).style.background=selMenuBgColor;

        var lastclickmenu  = parseInt(iIndex)+1;
        lastclickmenu = "pop" + lastclickmenu;
        $.cookie("lastclickmenu",lastclickmenu+";"+szID+";"+szUrl);
    }
    homeFrame.src = szUrl;
}

init_auth = function(){

   //隐藏串口设置
   document.getElementById("sub1_6").style.display = "none";

    if($.cookie("graintype") == 0){
      $("#menu4").hide();
      document.getElementById("sub2_2").style.display = "none";
      document.getElementById("sub2_3").style.display = "none";
    }

    if($.cookie("has_audio") == 1){
       document.getElementById("sub3_6").style.display = "block";
    } 

    if($.cookie("has_dome") == 1){
       document.getElementById("sub3_7").style.display = "none";
       document.getElementById("sub5_7").style.display = "none";
    }

    if($.cookie("has_dome") == 1 && $.cookie("shelter") == 0){
       document.getElementById("sub3_4").style.display = "none";
    }
}


getPlatform = function(){
    GetJCP({cmd: "version -act list", ParseJCP: function(jcpGet){
        if(jcpGet !== "Error"){
            var platform = typeof(jcpGet.platform)== 'undefined'?"normal":jcpGet.platform;
            platform = platform.toLowerCase();
            if(platform.charAt(platform.length-1)===','){
                platform = platform.substring(0,platform.length-1);
            }

            //如果平台没有包含"p2p","hbp2p"中的至少一个,屏蔽平台设置菜单
            //var arrPF = platform.split(",");
            var arr = [];
            /*for(var i=0,l=arrPF.length;i<l;i++){
                if(g_platform.indexOf([arrPF[i]]) != -1){
                    arr.push(arrPF[i]);
                    var subId = "";
                    switch(arrPF[i]){
                        case "guobiao":subId = "sub7_1";break;
                        case "hbp2p":subId = "sub7_2";break;
                        default:break;

                    }
                    document.getElementById(subId).style.display = "block";
                }
            }*/
            
            if (g_platform.indexOf(platform) != -1) {
                arr.push(platform);
                switch(platform){
                    case "hbgbp2p":
                    	document.getElementById("sub7_2").style.display = "block";
                    	break;
                    case "hbptp":
                    	document.getElementById("sub7_1").style.display = "block";
                    	document.getElementById("sub7_2").style.display = "block";
                    	break;
                    case "gbptp":
                    	document.getElementById("sub7_1").style.display = "block";
                    	document.getElementById("sub7_2").style.display = "block";
                    	break;
                    case "hbgbptp":
                    	document.getElementById("sub7_1").style.display = "block";
                    	document.getElementById("sub7_2").style.display = "block";
                    	break;
                }	
            }
            if(arr.length === 0){
                $("#menu7").hide();
            }
            
        }else{
            $("#menu7").hide();
        }
    }});
}

function diabledPage(){
   $("body").append('<div style="z-index:1;background-color:#FFF;filter: alpha(opacity=50);-moz-opacity:0.5;-khtml-opacity: 0.5;opacity: 0.5;width:100%;height:100%; position:absolute;left:0px;top:0px; "></div>'); 
}

function paramSaveTip(msg){
  $("#paramSaveTip").html(msg);
  if($("#paramSaveTip").is(":hidden")){
    $("#paramSaveTip").show();
    setTimeout(function(){document.getElementById("paramSaveTip").style.display="none";},2000);
  }
}
function paramFailTip(msg){
  $("#paramFailTip").html(msg);
  if($("#paramFailTip").is(":hidden")){
    $("#paramFailTip").show();
    setTimeout(function(){document.getElementById("paramFailTip").style.display="none";},2000);
  }
}
