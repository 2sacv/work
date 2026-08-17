var g_has_ocx = true;
var g_original_scale = true;
$(function(){
      var lf = $.cookie("loginflag_"+g_hostname);
      if (null === lf || typeof(lf) =='undefined' || lf === "null" || 0 > parseInt(lf)){
          window.location.href = "/login.asp";
      } else {
        window.document.title = IDC_MAINVIEW_TITLE;
        showLogo(); //显示顶部logo及设备型号

        if($.cookie("graintype") == 0){
          $("#spanPlayback").hide();
        }
        enable3DorSpeak(); //是否3D对讲
        init_call();
        _player();
        _init_click();
        init_ocxversion();
        stream_select();
        reRecordSet();
        initVideoSetting();
        setTimeShow();
      }
});

function setTimeShow(){
  document.getElementById('spanCurrTime').innerHTML = date();
  setTimeout(function(){setTimeShow();},1000);
}

//播放设置
function savePlaySetting(){
    var types = $("#playSettingFrame").contents().find("input[name='typelj']:checked").val();
    var playmode = $("#playSettingFrame").contents().find("input[name=playMode]:checked").val();  
    Set_cookie("playMode",playmode);
    Set_cookie("ljtypes",types);
    if(g_has_ocx){
        IPCamera.IPCSetFluency(0,parseInt(playmode));
        var stream = lastStream==0?"stream2":"stream1";
        IPCamera.IPCStartPreviewEx(0, GetCookieByKey("url"), 0, parseInt(types), GetCookieByKey("rtspport"),GetCookieByKey("user"), GetCookieByKey("passwd"), stream, "V2.00"); 
    }
    $("#playSetting").hide();
}


_player_size = function(){
 
  var $w = $(window).width();
  var $h = $(window).height();
  
  var ui_w = $w - 275;
  var ui_h = $h - 125;
  var stream_w = $.cookie("playsize_width");
  var stream_h = $.cookie("playsize_height");
  var width = stream_w*ui_h/stream_h;
  var height = ui_h;
  var top  = 0;
  //如果视频宽度+参数窗口宽度>当前窗口宽度*90%，则调整视频宽度
  if(width > ui_w){
     width = ui_w;
     height = stream_h*width/stream_w;
     top = (ui_h-height)/2;
  }

  //设置视频宽高及margin-top
  $("#object").css("width",width);
  $("#IPCamera").css({"width": width-2,"height": (height),"margin-top":top});
  $("#object").css("margin-left",(ui_w-width)/2);

  
}

_player_size_full_window = function(){
  var $w = $(window).width();
  var $h = $(window).height();
  var ui_w = $w - 275;
  var ui_h = $h - 125;
  $("#object").css("width",ui_w);
  $("#IPCamera").css({"width": ui_w,"height": ui_h});
  $("#object").css("margin-left",0);
}

_player = function(){
    if(g_is_msie){
      $("#object").html('<object id="IPCamera" name="IPCamera" CLASSID="CLSID:2319F6E6-ABD3-4b68-BADF-05D8796FA072"></object>');
    }else{
      $("#object").html('<object id="IPCamera" name="IPCamera" type="application/npipcam" width="100%" height="100%"></object>');
    }
}

var lastStream = 1;
stream_select = function(){
    var ms = GetCookieByKey("master_stream");
    var ss = GetCookieByKey("slave_stream");
    if(ms >= 0){
        $("#master_span").html(stream[ms]);
    }
    if(ss >= 0){
        $("#slave_span").html(stream[ss]);
    }

    if(parseInt(GetCookieByKey("master_enb")) == 1 && parseInt(GetCookieByKey("slave_enb")) == 1){
      if(GetCookieByKey("main_stream") == 'stream2'){
        $("#slave").attr("checked",true);
        player_slave_size();
        playing("stream2");
        lastStream = 0;
      }
      else
      {
        $("#master").attr("checked",true);
        player_master_size();
        playing("stream1");
        lastStream = 1;
      }
    }
    else{
        $("#slave").hide();
          $("#slave_span").hide();
          $("#master").attr("checked",true);
          player_master_size();
          playing("stream1");
          lastStream = 1;
    }
  
}

_init_click = function(){
      $("#full").click(function(){ 
          if(zoomFlag == 1){
                zoomFlag = 0;
                $("#zoom").attr("src","/image/mainview/area-full.png");
                $("#zoom").attr("title",IDC_ZOOM);
                document.IPCamera.IPCToggleAreaZoom(false); 
           }
		       if(_position_3d_flag == 1){
             _position_3d_flag = 0;
             $("#positions").attr('src','/image/mainview/3d.png');
             $("#positions").attr("title",IDC_3D_START);
             document.IPCamera.IPCToggle3DPositioning(IDC_3D_START, false);
           }
           document.IPCamera.IPCFullScreen(1); 
      });

      $("#originalScale").click(function(){
        if(!g_original_scale){
          g_original_scale = true;
          _player_size();
          $("#originalScale").attr("src","/image/mainview/original_scale_sel.png");
          $("#fullWindow").attr("src","/image/mainview/full_window.png");
        }
      });

      $("#fullWindow").click(function(){
        if(g_original_scale){
          g_original_scale = false;
          _player_size_full_window();
          $("#originalScale").attr("src","/image/mainview/original_scale.png");
          $("#fullWindow").attr("src","/image/mainview/full_window_sel.png");
        }
      });

      $("#screen").click(function(){ snap(); })
      $("#zoom").click(function(){ AreaZoom(); })
      $("#video").click(function(){ Video(); })
      $("#alarm").click(function(){ DoAlarm(this); });
      $("#setting").click(function(){ SettingPlay(this);});
      $("#speak").click(function(){ Speak(this);});
      $("#volume").click(function(){Volume(this);});
      $("#bottom_img img").hover(function(){ $(this).css("background","#3b3c3d"); })
      $("#bottom_img img").mouseleave(function(){ $(this).css("background","#0d0d0e"); })

      $(".side span").click(function(){
          switch($(this)[0].id){
            case "spanExit":
              if(confirm(IDC_MSGBOX_MSG)){
                deleteCookie("loginflag_"+g_hostname);
                location.href = "../login.asp"
              }
              break;
            case "spanPlayback":
              location.href = "playback.asp";
              break;
            case "spanSetting":
              location.href = "setting.asp"   
              break;
            case "spanLog":
              location.href = "log.asp";
              break;
            case "spanLiveview":
              break;
            default:
              break;
          }
      });

      $(".ptz_tab").click(function(){
        var _id = $(this)[0].id;
        if($("#"+_id+"Tab").is(":hidden")){
          $("#"+_id+"Tab").show();
          $("#"+_id+"Img").attr("src","/image/mainview/down.png");
        }else{
          $("#"+_id+"Tab").hide();
          $("#"+_id+"Img").attr("src","/image/mainview/up.png");
        }
      });

  $("input[name='stream']").click(function(){
      var str  = $('input:radio[name="stream"]:checked').val();
      if(str == lastStream)
      {
        window.focus();
      }else{
          if(zoomFlag == 1){
             zoomFlag == 0;
             document.IPCamera.IPCToggleAreaZoom(false);
             $("#zoom").attr("src","/image/mainview/area-full.png");
           $("#zoom").attr("title",IDC_ZOOM);
          }
		   if(_position_3d_flag == 1){
             _position_3d_flag = 0;
             $("#positions").attr('src','/image/mainview/3d.png');
             $("#positions").attr("title",IDC_3D_START);
             document.IPCamera.IPCToggle3DPositioning(IDC_3D_START, false);
           }
          if(recordFlag == 1 || alarmRecordFlag==1){
              recordFlag = 0;
              alarmRecordFlag = 0;
                document.IPCamera.IPCStopRecord(0);
            $("#video").attr("src", "/image/mainview/no-record.png");
            $("#video").attr("title", IDC_VIDEO);
          }
          if(g_sound_flag){
          g_sound_flag = false;
          document.IPCamera.IPCSetAudioRender(0, false);
          $("#volume").attr("src","/image/mainview/volume_close.png").attr("title",IDC_VOLUME_OPEN);
        }
        if(g_speak){
          g_speak = false;
          document.IPCamera.IPCStartAudioBroadCast(0, false);
          $("#speak").attr("src","/image/mainview/speak.png").attr("title",IDC_SPEAK_START);
        }
        lastStream = str;
        var main_stream = str == 1 ? "stream1" : "stream2";
        Set_cookie("main_stream",main_stream);
        play_stream();
		    document.IPCamera.IPCStopPreview(0);
		    window.setTimeout(function(){
          playing(main_stream);
        },300);
      }
  })
}

playing = function(stream){
    if(g_original_scale){
      _player_size();
    }else{
      _player_size_full_window();
    }
    
    if(g_has_ocx){
      IPCamera.IPCSetWindowMode(1);
      var mode = GetCookieByKey("playMode");
      mode = mode== -1?2:mode;
      var type = GetCookieByKey("ljtypes");
      type = type== -1?1:type;
      IPCamera.IPCSetFluency(0,parseInt(mode));
      IPCamera.IPCSetLanguage(GetCookieByKey("languages")==0?1:0);
      IPCamera.IPCStartPreviewEx(0, GetCookieByKey("url"), 0, parseInt(type), GetCookieByKey("rtspport"),GetCookieByKey("user"), GetCookieByKey("passwd"), stream, "V2.00"); 
    }
}

init_ocxversion = function(){
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
      beforeLocalRecordOrSnap();
      var version = document.IPCamera.IPCGetVersion(0).split("=")[1].split(";")[0];
      Set_cookie("ocxversion",version);
    }else{
      $("#object").html('<div align="center" style="font-size:14px;margin-top:'+($("#object").height()/2)+'px">'+IDC_OCX_NOTINSTALL+' '+IDC_DOWNLOAD+IDC_CLOSE_BROWSER+'</div>');
    }
}

init_call = function(){
  $("#laLiveview").html(IDC_PLAYVIDEO);
  $("#laLog").html(IDC_LOG);
  $("#laSetting").html(IDC_PARAMETER_SET);
  $("#laPlayback").html(IDC_PLAYBACK);
  $("#laExit").html(IDC_EXIT);

  $("#laColor").html(IDC_COLORPARAMETER);

  $("#imgbrightness").attr("title",IDC_BRIGHT.replace(/&nbsp;/g,'').replace(':','').replace('：',''));
  $("#imgcontrast").attr("title",IDC_CONTRAST.replace(/&nbsp;/g,'').replace(':','').replace('：',''));
  $("#imgsaturation").attr("title",IDC_SATURATION.replace(/&nbsp;/g,'').replace(':','').replace('：',''));
  $("#imgsharpness").attr("title",IDC_SHRPNESS.replace(/&nbsp;/g,'').replace(':','').replace('：',''));

  $("#video").attr("title",IDC_VIDEO);
  $("#zoom").attr("title",IDC_ZOOM);
  $("#full").attr("title",IDC_FULL);
  $("#screen").attr("title",IDC_SNAP);
  $("#fullWindow").attr("title",IDC_FULL_WINDOW);
  $("#originalScale").attr("title",IDC_ORIGINAL_SCALE);
  $("#alarm").attr("title",IDC_ALARM);
  $("#setting").attr("title",IDC_PLAYMODE_PLAYSETTING);
  $("#positions").attr("title",IDC_3D_START);
  $("#speak").attr("title",IDC_SPEAK_START);
  $("#volume").attr("title",IDC_VOLUME_OPEN);
  $("#ptzShow").attr("title",IDC_PTZ_SHOW);
  $("#playSettingFrame").attr("src","/asp/playsetting.asp");
}

var recordFlag = 0; //0没有录像，1正在录像
var alarmRecordFlag = 0;//0没有报警录像，1正在报警录像
var manualRecordFlag = 0; //报警前手动录像标志
var alarmTimeHandle = 0;
Video = function(){
  if(alarmRecordFlag == 1 || recordFlag == 1)
  {
    $("#video").attr("src", "/image/mainview/no-record.png");
    $("#video").attr("title", IDC_VIDEO);
    
    stopRecord();
  }
  else
  {
    $("#video").attr("src", "/image/mainview/recording.png");
    $("#video").attr("title", IDC_VIDEOING);
    
    recordFlag = 1;
    recording();
  }
}

var g_speak = false; 
Speak = function(obj){
  if(g_speak){
      g_speak = false;
      $("#speak").attr("src","/image/mainview/speak.png");
      $("#speak").attr("title",IDC_SPEAK_START);
      document.IPCamera.IPCStartAudioBroadCast(0, false);
  }else{
      g_speak = true;
      $("#speak").attr("src","/image/mainview/speakSel.png");
      $("#speak").attr("title",IDC_SPEAK_STOP);
      document.IPCamera.IPCStartAudioBroadCast(0, true);
  }
}

var g_sound_flag = false; //默认关闭
function Volume(obj)
{
  if (g_sound_flag){
      $("#volume").attr("src","/image/mainview/volume_close.png").attr("title",IDC_VOLUME_OPEN);
      parent.document.IPCamera.IPCSetAudioRender(0, false);
      g_sound_flag = false;
  } else{
      $("#volume").attr("src","/image/mainview/volume.png").attr("title",IDC_VOLUME_CLOSE);
      parent.document.IPCamera.IPCSetAudioRender(0, true);
      g_sound_flag = true;
  }
}

function reRecordSet(){
  if(1 == GetCookieByKey("PreRecCk")){
    var reRecordTime = parseInt(GetCookieByKey("PreRecTime"));
         reRecordTime = reRecordTime == -1 ? 5 : reRecordTime;
    document.IPCamera.IPCSetPreRecord(0, true, reRecordTime);
  }
}

function recording(){
  manualRecordFlag = 0;
  var record_type = 'Manual';
  var path = LocalPath+"\\record\\" + document.domain + "\\"+ video_date() + "\\" + record_type + "-"+ video_times() + ".mp4";

  if (true == document.IPCamera.IPCIsRecording(0)){
    if(document.IPCamera.IPCStopRecord(0)){
        if(!document.IPCamera.IPCStartTimerRecord(0,path,LocalRecordTime)){
		    manualRecordFlag = 0;
			recordFlag = 0;
  	        $("#video").attr("src", "/image/mainview/no-record.png");
  	        $("#video").attr("title", IDC_VIDEO);
			paramSaveTip(IDC_RECORD_ERROR_DISK_FULL);
		}else{
			paramSaveTip(IDC_RECORD_MANNUAL_FILE + path);
		}
    }
  }else{
        if(!document.IPCamera.IPCStartTimerRecord(0,path,LocalRecordTime)){
		    manualRecordFlag = 0;
			recordFlag = 0;
  	        $("#video").attr("src", "/image/mainview/no-record.png");
  	        $("#video").attr("title", IDC_VIDEO);
			paramSaveTip(IDC_RECORD_ERROR_DISK_FULL);
		}else{
			paramSaveTip(IDC_RECORD_MANNUAL_FILE + path);
		}
  }
}

function stopRecord(){
  window.clearTimeout(alarmTimeHandle);
	if ("undefined" == typeof(document.IPCamera.IPCIsRecording))
  {
      return;
  }
  if (true == document.IPCamera.IPCIsRecording(0))
  {
      var ret;
      if(alarmRecordFlag == 1){
         ret = document.IPCamera.IPCStopRecord(0);
         paramSaveTip(IDC_STOP_ALARM_RECORD);
      }else{
          ret = document.IPCamera.IPCStopTimerRecord(0);
          paramSaveTip(IDC_STOP_MANUAL_RECORD);
      }
      if(!ret)
      {
          paramSaveTip(IDC_RCRD_STOP_FAIL);
          window.focus();
      }
  }
  recordFlag = 0;
  alarmRecordFlag = 0;
  manualRecordFlag = 0;
}

function alarmRecord(){
  if(alarmRecordFlag == 0){
    //告警录像抢断手动录像，告警录像后继续开始之前的手动录像
    var AlarmRecCk = GetCookieByKey("AlarmRecCk");
    if(AlarmRecCk == 1){
        var AlarmRecTime = GetCookieByKey("AlarmRecTime");
        if(AlarmRecTime == -1){
          AlarmRecTime = 20;
        }
      
        var record_type = 'Alarm';
        var path = LocalPath+"\\record\\" + document.domain + "\\"+ video_date() + "\\" + record_type + "-"+ video_times()+".mp4";
        alarmRecordFlag = 1;
        //手动录像停止
        if(recordFlag == 1 && manualRecordFlag == 0){
          var ret = document.IPCamera.IPCStopTimerRecord(0);
          if(ret){
            manualRecordFlag = 1;
			
            if(document.IPCamera.IPCStartRecord(0,path)){
			    $("#video").attr("src", "/image/mainview/recording.png");
                $("#video").attr("title", IDC_ALARM_VIDEOING);
				alarmTimeHandle = window.setTimeout("stopAlarmRecord()",AlarmRecTime*1000);
			}else{
			    manualRecordFlag = 0;
				alarmRecordFlag = 0;
				recordFlag = 0;
				paramSaveTip(IDC_RECORD_ERROR_DISK_FULL);
			};
          }
         
        }else{
             if(document.IPCamera.IPCStartRecord(0,path)){
			    $("#video").attr("src", "/image/mainview/recording.png");
                $("#video").attr("title", IDC_ALARM_VIDEOING);
				alarmTimeHandle = window.setTimeout("stopAlarmRecord()",AlarmRecTime*1000);
			}else{
			    manualRecordFlag = 0;
				alarmRecordFlag = 0;
				recordFlag = 0;
				paramSaveTip(IDC_RECORD_ERROR_DISK_FULL);
			};
        }
       
    }
  }
  
}

function stopAlarmRecord(){
  window.clearTimeout(alarmTimeHandle);
  if(alarmRecordFlag == 1){
        alarmRecordFlag = 0;
        paramSaveTip(IDC_STOP_ALARM_RECORD);
        var ret = document.IPCamera.IPCStopRecord(0);
        if(ret){
          //如果报警前手动录像打开，则继续手动录像
          if(manualRecordFlag == 1){
            $("#video").attr("title", IDC_VIDEOING);
            recording();
          }else{
            $("#video").attr("src", "/image/mainview/no-record.png");
            $("#video").attr("title", IDC_VIDEO);
          }
        }
    }
}

var LocalPath;//存储路径
var LocalRecordTime=0;//存储时间
function beforeLocalRecordOrSnap(){
  LocalPath = GetCookieByKey("RecPath");
    if (LocalPath == -1)
    {
        LocalPath = "D:\\IPCamera";
    }
   
    LocalRecordTime = GetCookieByKey("RecPackTime");
    if (LocalRecordTime == -1)
    {
      LocalRecordTime = 10;
    }
    LocalRecordTime = LocalRecordTime*60;
}

window.onbeforeunload = function(){
  if(recordFlag == 1 || alarmRecordFlag == 1){
      stopRecord();
  }
  if(g_speak){
      g_speak = false;
      document.IPCamera.IPCStartAudioBroadCast(0, false);
  }
}

var zoomFlag=0; //放大选择标志 0未选择，1选择

AreaZoom = function(){
  if(zoomFlag == 1){
    zoomFlag = 0;
    document.IPCamera.IPCToggleAreaZoom(false); 
    $("#zoom").attr("src","/image/mainview/area-full.png");
    $("#zoom").attr("title",IDC_ZOOM);
  }else{
    //3D定位开启的关闭
    if(_position_3d_flag == 1){
      _position_3d_flag = 0;
      $("#positions").attr('src','/image/mainview/3d.png');
      $("#positions").attr("title",IDC_3D_START);
    }
    zoomFlag = 1;
    document.IPCamera.IPCToggleAreaZoom(true);
    $("#zoom").attr("src","/image/mainview/area-fulling.png");
    $("#zoom").attr("title",IDC_ZOOMING);
  }
    
}

function snap()
{
      var id = '1';
      var stram_path = "stream1";
      var path = LocalPath+"\\snapshots\\" + video_date() + "\\" + document.domain + "-" + id + "-" + stram_path + "-" + video_times()+".jpg"
      var a = document.IPCamera.IPCSnapPic(0,path)
      if(a)
      {
        paramSaveTip(IDC_SNAP_FILE + path);
      }else{
        paramSaveTip(IDC_SNAP_ERROR_DISK_FULL);
      }
}

var _position_3d_flag = 0; 
function positions3d(){
  if(_position_3d_flag == 0){
        //关闭放大
        if(zoomFlag == 1){
          zoomFlag = 0;
          $("#zoom").attr("src","/image/mainview/area-full.png");
          $("#zoom").attr("title",IDC_ZOOM);
        }
        _position_3d_flag = 1;
        $("#positions").attr('src','/image/mainview/3D_sel.png');
        $("#positions").attr("title",IDC_3D_STOP);
        document.IPCamera.IPCToggle3DPositioning(IDC_3D_START, true);
  } 
  else if (_position_3d_flag == 1)
  {
      _position_3d_flag = 0;
      $("#positions").attr('src','/image/mainview/3d.png');
      $("#positions").attr("title",IDC_3D_START);
      document.IPCamera.IPCToggle3DPositioning(IDC_3D_START, false);
  }
}

video_date = function(){
    var now = new Date();
    var strDate = now.getFullYear();
    if ((1 + now.getMonth()) < 10)
    {// 前面补零，比如 1 -> 01
        strDate += '0';
    }
    strDate += (1 + now.getMonth()).toString();
    
    if (now.getDate() < 10)
    {
        strDate += '0';
    }
    strDate += now.getDate();
    return strDate;
}

//报警自动触发操作
function triggerAlarm(alarmMsg){
    //报警录像
    alarmRecord();
    //报警信息展示
    alarmInfoShow(alarmMsg);
}

function triggerZoomRectCanceled(chn){
   zoomFlag = 0;
   document.IPCamera.IPCToggleAreaZoom(false);  
   $("#zoom").attr("src","/image/mainview/area-full.png");
   $("#zoom").attr("title",IDC_ZOOM);
}

var arrAlarmInfoData = [];//报警数量，保存最近20条
function alarmInfoShow(alarmMsg){
  $("#alarm").attr("src", "/image/mainview/alarming.gif");
  var alarmArr = alarmMsg.split(";");
  var type = alarmArr[0].split("=")[1];
  var date = alarmArr[1].split("=")[1];
  
  var html = "<tr>";
  html+="<td align='center' width='40%' style='border:1px solid #666;'>"+ALARM_TYPE_ARR[type]+"</td>";
  html+="<td align='center' width='60%' style='border:1px solid #666;'>"+date+"</td></tr>";

  if(arrAlarmInfoData.length >= 20){
    arrAlarmInfoData.splice(0,1);
    arrAlarmInfoData[19] = html;
  }else{
    arrAlarmInfoData.push(html);
  }

  var $tbmsg = $("#containmsgframe").contents().find("#msgtable");
  var temp = "";
  for(var i=arrAlarmInfoData.length-1;i>=0;i--){
    temp += arrAlarmInfoData[i];
  }

  $tbmsg.empty().append(temp);
}


//点击报警图标显示报警信息
function DoAlarm(obj){
  $("#alarm").attr("src", "/image/mainview/no-alarm.png");
  var para = $("#" + obj.id).offset();
  if ($("#containmsg").css("display") == "block")
    {
        $("#containmsg").hide();
    }
    else
    {
        $("#playSetting").hide();
        $("#containmsg").css({'top': para.top - 220 + 'px','left': (para.left - 320) + 'px'});
        $("#containmsg").show();
    }
}

//播放选项设置
function SettingPlay(obj){
  if ($("#playSetting").css("display") == "block"){
      $("#playSetting").hide();
  }else{
      $("#containmsg").hide();
      var para = $("#" + obj.id).offset();
      $("#playSetting").css({'top': para.top - 220 + 'px','left': (para.left - 230) + 'px'});
      $("#playSetting").show();
  }
}

$(window).resize(function(){
  _player_size();
});


function showLogo(){
  var $logo = $("#top_logo_image");
  $logo.attr("src","/image/logo.png");
  $logo.show();
  GetJCP({cmd: "version -act list",ParseJCP: function(result){
     if(result != 'Error'){
        $("#top_title_span").html(result.devtype); //tag_devtype
        Set_cookie("dome_modle",result.devtype);
     }
  }});
}

function enable3DorSpeak(){
  if($.cookie("has_3d")==1){
    $("#positions").show();
  }

  if($.cookie("has_audio")==1){
    $("#speak").show();
    $("#volume").show();

    setTimeout(function(){enableSpeakVolume();},2000);
  }
}

function enableSpeakVolume(){
    $("#volume").attr("src","/image/mainview/volume.png").attr("title",IDC_VOLUME_CLOSE);
    document.IPCamera.IPCSetAudioRender(0, true);
    g_sound_flag = true;
}

function FireChannelEvent(msg,chn,str){
  if(msg == 12){ //报警
      triggerAlarm(str);
  }else if(msg == 13){
      triggerZoomRectCanceled(chn);
  }else if(msg == 14){
    stopRecordWhenDiskIsFull();
  }
    
}

function stopRecordWhenDiskIsFull(){
  $("#video").attr("src", "/image/mainview/no-record.png");
  $("#video").attr("title", IDC_VIDEO);
  stopRecord();
  paramSaveTip(IDC_RECORD_ERROR_DISK_FULL);
}

//双击会执行两次onmouseup事件，引起问题
var m_oTime = 0, m_nTime = 0;
_cleam_nTime = function(){
  m_oTime = 0;
  m_nTime = 0;
}

function paramSaveTip(msg){
  $("#paramSaveTip").html(msg);
  if($("#paramSaveTip").is(":hidden")){
    $("#paramSaveTip").show();
    setTimeout(function(){document.getElementById("paramSaveTip").style.display="none";},2000);
  }
}

var bright, contrast, saturation, sharpness;
function initVideoSetting(){
    try
    {
        bright = new SliderModules({
                targetId: "bright",
                min: 1,
                max: 255
              }); 
              bright.create();
              bright.onchange = function () {
                  $('#tdBright').text(bright.getValue());
                  GetJCP({cmd: "vicfg -act set  -bright " + bright.getValue()});
              };

        contrast = new SliderModules({
                targetId: "contrast",
                min: 1,
                max: 255
              }); 
              contrast.create();
              contrast.onchange = function () {
                  $('#tdContrast').text(contrast.getValue());
                  GetJCP({cmd: "vicfg -act set  -contrast " + contrast.getValue()});
              };


        
        saturation = new SliderModules({
                targetId: "saturation",
                min: 1,
                max: 255
              }); 
              saturation.create();
              saturation.onchange = function () {
                  $('#tdSaturation').text(saturation.getValue());
                  GetJCP({cmd: "vicfg -act set -saturation " + saturation.getValue()});
              };

        
        sharpness = new SliderModules({
            targetId: "sharpness",
            min: 1,
            max: 255
          }); 
          sharpness.create();
          sharpness.onchange = function () {
              $('#tdSharpness').text(sharpness.getValue());
              GetJCP({cmd: "vicfg -act set -sharpness " + sharpness.getValue()});
          };

        GetJCP({cmd: "vicfg -act list ", ParseJCP: ParseVICfg});

    }
    catch(E){return E;}
}

//初始化图像设定-颜色参数信息
function ParseVICfg(jcpstr)
{
    try
    {
        bright.wsetValue(parseInt(jcpstr.bright));
        contrast.wsetValue(parseInt(jcpstr.contrast));
        saturation.wsetValue(parseInt(jcpstr.saturation));
        sharpness.wsetValue(parseInt(jcpstr.sharpness));

       
        $('#tdBright').text(jcpstr.bright);
        $('#tdContrast').text(jcpstr.contrast);
        $('#tdSaturation').text(jcpstr.saturation);
        $('#tdSharpness').text(jcpstr.sharpness);
    }
    catch(e){}
}