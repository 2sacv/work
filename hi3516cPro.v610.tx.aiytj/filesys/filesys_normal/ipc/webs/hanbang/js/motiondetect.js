function initMD(){
    _init_player()
    _init_load()
    _init_click()
    _init_slider()
}

var empty = "0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,";
_init_player = function(){
    if(g_is_msie){
      $("#motion_ipcamer").html('<object id="IPCamera" name="IPCamera" CLASSID="CLSID:2319F6E6-ABD3-4b68-BADF-05D8796FA072" width="500" height="370"></object>');
    }else{
      $("#motion_ipcamer").html('<object id="IPCamera" name="IPCamera" type="application/npipcam" width="500" height="370"></object>');
    }
    document.IPCamera.IPCSetWindowMode(1);
    var type = GetCookieByKey("ljtypes");
    type = type== -1?1:type;
    document.IPCamera.IPCStartPreviewEx(0, GetCookieByKey("url"), 0, parseInt(type), GetCookieByKey("rtspport"),GetCookieByKey("user"), GetCookieByKey("passwd"), GetCookieByKey("stream"),"V2.00");
}

var g_desc_info = "";
IPCWndInit = function()
{
    GetJCP({cmd: "mdmbcfg -act list",ParseJCP: function(info){
        if(info!='Error'){
            $("#m_slider_span").html(info.thresh)
            $('#m_slider').slider("value",info.thresh)
            $("#motion_time_protection").selectTime('setData',info.timestrategy)
            if(info.enable == '1'){
              $("#motion_enb").prop("checked",true)
            }
            g_desc_info = info.mbdesc;
            if(false == document.IPCamera.IPCSetMDMode(0, true)){ return; }
            window.setTimeout(function(){
                IPCamera.IPCSetMDInfo(0, 1, g_desc_info);
            },200);
        }
    }});
    
}

_init_click = function(){
  $("#m_select").click(function(){ 
      IPCamera.IPCSetMDModeEx(0,true);
      IPCamera.IPCSetMDInfo(0, 0,"");
  })

  $("#m_setting").click(function(){
    var mbdesc = IPCamera.IPCGetMDInfo(0);
    IPCamera.IPCSetMDInfo(0, 1, mbdesc); 
    var enb = $("#motion_enb").is(":checked") == true ? 1 : 0
    var str = "mdmbcfg -act set -timestrategy " + $("#motion_time_protection").selectTime('getData');
    str += " -mbdesc " + IPCamera.IPCGetMDInfo(0) + " -enable " + enb + " -thresh " + $("#m_slider_span").html();
 
    GetJCP({cmd:str , ParseJCP: function(result){
        if(result == "Error"){
          parent.paramFailTip(IDC_MSGBOX_SAVEFAIL);
        }else{
          parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
        }
        window.focus();
    }});
  })

  $("#m_delect").click(function(){
    document.IPCamera.IPCSetMDInfo(0, 0, empty)
    GetJCP({cmd:"mdmbcfg -act set -mbdesc " + empty, ParseJCP: function(result){
        if(result == "Error"){
          parent.paramFailTip(IDC_MSGBOX_DELETEFAIL);
        }else{
          parent.paramSaveTip(IDC_MSGBOX_DELETEOK);
        }
    }})
    window.focus();
  })
}

_init_slider = function(){
      $('#m_slider').slider({
        min: 1,
        max: 100,
        slide: (function(event, ui) {
            $("#m_slider_span").html(ui.value);
        })
      });
}

_init_load = function(){
  $("#motion_lab").html(IDC_MOTIONDETECT);
  $("#motiontp_lab").html(IDC_TIME_PROTECTION);
  $("#motion_tp_tip").html(IDC_TIME_PROTECTION);
  $("#m_select").html(IDC_SELECT_TABLE);
  $("#m_show").html(IDC_SHOW);
  $("#m_setting").html(IDC_SAVE)
  $("#m_delect").html(IDC_DEL)
  $("#m_save").html(IDC_SAVE);
  $("#ms_span").html(IDC_MSENSITIVITY);
  $("#select_prompt").html(IDC_SELECT_PROMPT);
  $("#motion_enb_lab").html(IDC_MD_ENABLE)
  $("#motion_time_protection").selectTime();
}

function FireChannelEvent(msg,chn, str){
  if(msg == 11){
    IPCWndInit();
  }else if(msg == 12){
    handleMotionDetect(str);
  }
}

function handleMotionDetect(alarm_msg){
  if("MOTIONDETECT"==alarm_msg.split(";")[0].split("=")[1]){
    $("#mdTip").html(IDC_MOTIONDETECT_ALARM);
    window.setTimeout(function(){
      $("#mdTip").html('');
    },2000);
  }
}

var g_curr_tab = "IDC_MENU_MOTION";
function reloadVideo(){
  if(!g_is_msie && g_curr_tab != "IDC_MENU_MOTION"){
    window.location.href = window.location.href;
  }
}