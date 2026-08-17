var old_dir = 0; //原来的方向
var wndWidth = 500; //视频真实宽度
var wndHeight = 370; //视频真实高度
var veWidth=1920, veHeight=1080; //用插件获取到的尺寸
var initVideoShow = 0;

function initMD(){
    var _cookie_last_menu = $.cookie("alarm_last_menu");
    if (null === _cookie_last_menu || typeof(_cookie_last_menu) =='undefined' || -1 == _cookie_last_menu || _cookie_last_menu == "IDC_MF_ALARM") {
        g_curr_tab = "IDC_MF_ALARM";
        $("#liMF").addClass("ui-tabs-active ui-state-active");
        $("#liPeople").removeClass("ui-tabs-active ui-state-active");
        $("#liArea").removeClass("ui-tabs-active ui-state-active");
        $("#liMD").removeClass("ui-tabs-active ui-state-active");
        $("#liCarDetect").removeClass("ui-tabs-active ui-state-active");
    } else if (_cookie_last_menu == "IDC_CROSS_ALARM") {
        g_curr_tab = "IDC_CROSS_ALARM";
        $("#liCross").addClass("ui-tabs-active ui-state-active");
        $("#liMF").removeClass("ui-tabs-active ui-state-active");
        $("#liMD").removeClass("ui-tabs-active ui-state-active");
        $("#liPeople").removeClass("ui-tabs-active ui-state-active");
        $("#liCarDetect").removeClass("ui-tabs-active ui-state-active");
    } else if (_cookie_last_menu == "IDC_AREA_ALARM") {
        g_curr_tab = "IDC_AREA_ALARM";
        $("#liArea").addClass("ui-tabs-active ui-state-active");
        $("#liMF").removeClass("ui-tabs-active ui-state-active");
        $("#liMD").removeClass("ui-tabs-active ui-state-active");
        $("#liPeople").removeClass("ui-tabs-active ui-state-active");
        $("#liCarDetect").removeClass("ui-tabs-active ui-state-active");
    } else if (_cookie_last_menu == "IDC_PEOPLE_ALARM") {
        g_curr_tab = "IDC_PEOPLE_ALARM";
        $("#liPeople").addClass("ui-tabs-active ui-state-active");
        $("#liArea").removeClass("ui-tabs-active ui-state-active");
        $("#liMF").removeClass("ui-tabs-active ui-state-active");
        $("#liMD").removeClass("ui-tabs-active ui-state-active");
        $("#liCarDetect").removeClass("ui-tabs-active ui-state-active");
    }  else if (_cookie_last_menu == "IDC_MF_ALARM") {
        g_curr_tab = "IDC_MF_ALARM";
        $("#liMF").addClass("ui-tabs-active ui-state-active");
        $("#liPeople").removeClass("ui-tabs-active ui-state-active");
        $("#liArea").removeClass("ui-tabs-active ui-state-active");
        $("#liMD").removeClass("ui-tabs-active ui-state-active");
        $("#liCarDetect").removeClass("ui-tabs-active ui-state-active");
    } else if (_cookie_last_menu == "IDC_PEOPLE_CAR_DETECT") {
        g_curr_tab = "IDC_PEOPLE_CAR_DETECT";
        $("#liCarDetect").addClass("ui-tabs-active ui-state-active");
        $("#liMF").removeClass("ui-tabs-active ui-state-active");
        $("#liPeople").removeClass("ui-tabs-active ui-state-active");
        $("#liArea").removeClass("ui-tabs-active ui-state-active");
        $("#liMD").removeClass("ui-tabs-active ui-state-active");
    }
    _init_slider();
    _init_Show_VideoInfo();
    _init_load();
    _init_click();
}

var empty = "0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,0000000000000000000000,";
_init_player = function(){
    if(g_is_msie){
      $("#motion_ipcamer").html('<object id="IPCamera" name="IPCamera" CLASSID="CLSID:2319F6E6-ABD3-4b68-BADF-05D8796FA072" width="' + wndWidth +'" height="' + wndHeight + '"></object>');
    }else{
      $("#motion_ipcamer").html('<object id="IPCamera" name="IPCamera" type="application/npipcam" width="' + wndWidth +'" height="' + wndHeight + '"></object>');
    }
    document.IPCamera.IPCSetWindowMode(1);
    var type = GetCookieByKey("ljtypes");
    type = type== -1?1:type;
    document.IPCamera.IPCStartPreviewEx(0, GetCookieByKey("url"), 0, parseInt(type), GetCookieByKey("rtspport"),GetCookieByKey("user"), GetCookieByKey("passwd"), GetCookieByKey("stream"),"V2.00");
}

_init_Show_VideoInfo = function(){
    if(initVideoShow==0){
        initVideoShow++
        _init_player();
    }
}

var g_desc_info = "";
IPCWndInit = function()
{
    if (g_curr_tab == "IDC_MENU_MOTION") {
        $("#cross_monitor_tab").hide();
        $("#area_monitor_tab").hide();
        $("#people_monitor_tab").hide();
        $("#follow_monitor_tab").hide();
        $("#car_detect_monitor_tab").hide();
        $("#m_btn_tab").show();
        GetJCP({cmd: "mdmbcfg -act list",ParseJCP: function(info){
            if(info!='Error'){
                $("#m_slider_span").html(info.thresh)
                $('#m_slider').slider("value",info.thresh)
                $("#motion_time_protection").selectTime('setData',info.timestrategy)
                if(info.enable == '1'){
                  $("#motion_enb").prop("checked",true);
                } else {
                  $("#motion_enb").prop("checked",false);
                }
                g_desc_info = info.mbdesc;
                if(false == document.IPCamera.IPCSetMDMode(0, true)){ return; }
                window.setTimeout(function(){
                    IPCamera.IPCSetMDInfo(0, 1, g_desc_info);
                },200);
            }
        }});
    } else if (g_curr_tab == "IDC_CROSS_ALARM") {
        $("#area_monitor_tab").hide();
        $("#follow_monitor_tab").hide();
        $("#m_btn_tab").hide();
        $("#people_monitor_tab").hide();
        $("#car_detect_monitor_tab").hide();
        $("#cross_monitor_tab").show();
        document.IPCamera.IPCSetVGMode(0, 1);
        GetJCP({cmd: "vgline -act list",ParseJCP: function(info){
            if(info!='Error'){
                $("#m_slider_span_cross_monitor").html(info.thresh)
                $('#m_slider_cross_monitor').slider("value",info.thresh)
                $("#cross_monitor_time_protection").selectTime('setData',info.timestrategy)
                if(info.enable == '1'){
                  $("#cross_monitor_enb").prop("checked",true);
                } else {
                  $("#cross_monitor_enb").prop("checked",false);
                }
                
                if(info.blink == '1'){
                  $("#cross_blink_enb").prop("checked",true);
                } else {
                  $("#cross_blink_enb").prop("checked",false);
                }

                $("#select_cross_scene_mode").val(info.indoor);
                

                do
                {
                    if (info.x0 == 0 && info.y0 == 0 && info.x1 == 0 && info.y1 == 0)
                    {
                        break;
                    }

                    var msg = "x0=" + setOCXShowXPosition(info.x0) + ",y0=" + setOCXShowYPosition(info.y0) + ",";
                    msg += "x1=" + setOCXShowXPosition(info.x1) + ",y1=" + setOCXShowYPosition(info.y1) + ",";
                    msg += "x2=" + setOCXShowXPosition(info.dx0) + ",y2=" + setOCXShowYPosition(info.dy0) + ",";
                    msg += "x3=" + setOCXShowXPosition(info.dx1) + ",y3=" + setOCXShowYPosition(info.dy1) + ",";
                    document.IPCamera.IPCSetVGMode(0, 1);
                    document.IPCamera.IPCSetVGInfo(0, msg);

                    var _dir = JudgeDirection(
                        new Point(info.x0, info.y0), 
                        new Point(info.x1, info.y1), 
                        new Point(info.dx0, info.dy0), 
                        new Point(info.dx1, info.dy1));

                    if (_dir == -1)
                    {
                        break;
                    } 
                    else 
                    {
                        old_dir = _dir;
                    }
                

                    $("#select_cross_direction").val(_dir);
                }
                while (0);
                
            }
        }});
    } else if (g_curr_tab == "IDC_AREA_ALARM") {
        $("#cross_monitor_tab").hide();
        $("#follow_monitor_tab").hide();
        $("#m_btn_tab").hide();
        $("#people_monitor_tab").hide();
        $("#car_detect_monitor_tab").hide();
        $("#area_monitor_tab").show();
        document.IPCamera.IPCSetVGMode(0, 2);

        GetJCP({cmd: "vgrect -act list",ParseJCP: function(info){
            if(info!='Error'){
                $("#m_slider_span_area_monitor").html(info.thresh)
                $('#m_slider_area_monitor').slider("value",info.thresh)
                $("#area_monitor_time_protection").selectTime('setData',info.timestrategy)
                if(info.enable == '1'){
                  $("#area_monitor_enb").prop("checked",true);
                } else {
                  $("#area_monitor_enb").prop("checked",false);
                }
                if(info.blink == '1'){
                  $("#area_blink_enb").prop("checked",true);
                } else {
                  $("#area_blink_enb").prop("checked",false);
                }
                
                $("#select_area_scene_mode").val(info.indoor);
                $("#select_area_direction").val(info.dir);

                if (info.x0 == 0 && info.y0 == 0 && info.x1 == 0 && info.y1 == 0) {
                    
                } else {
                    var msg = "x0=" + setOCXShowXPosition(info.x0) + ",y0=" + setOCXShowYPosition(info.y0) + ",";
                    msg += "x1=" + setOCXShowXPosition(info.x1) + ",y1=" + setOCXShowYPosition(info.y1) + ",";
                    msg += "x2=" + setOCXShowXPosition(info.x2) + ",y2=" + setOCXShowYPosition(info.y2) + ",";
                    msg += "x3=" + setOCXShowXPosition(info.x3) + ",y3=" + setOCXShowYPosition(info.y3) + ",";
                    document.IPCamera.IPCSetVGMode(0, 2);
                    document.IPCamera.IPCSetVGInfo(0, msg);
                }
            }
        }});
    } else if (g_curr_tab == "IDC_PEOPLE_ALARM") {
        $("#cross_monitor_tab").hide();
        $("#area_monitor_tab").hide();
        $("#follow_monitor_tab").hide();
        $("#m_btn_tab").hide();
        $("#car_detect_monitor_tab").hide();
        $("#people_monitor_tab").show();
        GetJCP({cmd: "humandetectcfg -act list",ParseJCP: function(info){
            if(info!='Error'){
                // $("#m_slider_span_env_sensitivity").html(info.thresh)
                // $('#m_slider_env_sensitivity').slider("value",info.thresh)

                $("#m_slider_span_body_distance").html(info.thresh)
                $('#m_slider_body_distance').slider("value",info.thresh)

                $("#people_monitor_time_protection").selectTime('setData',info.timestrategy)

                if(info.enable == '1'){
                  $("#people_monitor_enb").prop("checked",true);
                } else {
                  $("#people_monitor_enb").prop("checked",false);
                }

                if(info.screenenable == '1'){
                  $("#people_blink_enb").prop("checked",true);
                } else {
                  $("#people_blink_enb").prop("checked",false);
                }

                if(info.person_center == '1'){
                  $("#people_center_enb").prop("checked",true);
                } else {
                  $("#people_center_enb").prop("checked",false);
                }

                if(info.faceae == '1'){
                  $("#face_illumination_enb").prop("checked",true);
                } else {
                  $("#face_illumination_enb").prop("checked",false);
                }

                $("#select_people_scene_mode").val(info.outdoor);

                g_desc_info = info.mbdesc;
                if(false == document.IPCamera.IPCSetHDMode(0, true)){ return; }
                window.setTimeout(function(){
                    IPCamera.IPCSetHDInfo(0, g_desc_info);

                    var str = "humandetectcfg -act set ";
                    str += " -drag 1 ";
                    // str += " -humandistance " +  info.humandistance;

                    GetJCP({cmd:str , ParseJCP: function(result){}});
                },200);
            }
        }});
    } else if (g_curr_tab == "IDC_PEOPLE_CAR_DETECT") {
        $("#cross_monitor_tab").hide();
        $("#area_monitor_tab").hide();
        $("#follow_monitor_tab").hide();
        $("#m_btn_tab").hide();
        $("#people_monitor_tab").hide();
        $("#car_detect_monitor_tab").show();
        GetJCP({cmd: "cardetectcfg -act list",ParseJCP: function(info){
            if(info!='Error'){
                $("#m_slider_car_detect_sensitivity").slider("value",info.thresh);
                $('#m_slider_span_car_detect_sensitivity').html(info.thresh);

                $("#car_detect_monitor_time_protection").selectTime('setData',info.timestrategy)

                if(info.enable == '1'){
                  $("#car_detect_monitor_enb").prop("checked",true);
                } else {
                  $("#car_detect_monitor_enb").prop("checked",false);
                }

                if(info.screenenable == '1'){
                  $("#car_detect_blink_enb").prop("checked",true);
                } else {
                  $("#car_detect_blink_enb").prop("checked",false);
                }

                g_desc_info = info.mbdesc;
                if(false == document.IPCamera.IPCSetHDMode(0, true)){ return; }
                window.setTimeout(function(){
                    IPCamera.IPCSetHDInfo(0, g_desc_info);

                    var str = "cardetectcfg -act set ";
                    str += " -drag 1 ";
                    str += " -cardistance " +  info.cardistance;

                    GetJCP({cmd:str , ParseJCP: function(result){}});
                },200);
            }
        }});
    } else if (g_curr_tab == "IDC_MF_ALARM") {
        $("#cross_monitor_tab").hide();
        $("#area_monitor_tab").hide();
        $("#people_monitor_tab").hide();
        $("#car_detect_monitor_tab").hide();
        $("#m_btn_tab").hide();
        $("#follow_monitor_tab").show();
        document.IPCamera.IPCSetMDMode(0, true);
        GetJCP({cmd: "followcfg -act list",ParseJCP: function(info){
            if(info!='Error'){
                $("#preset").val(info.preset);
                $('#remainedtime').val(info.idle);
                $("#follow_time_protection").selectTime('setData',info.timestrategy)
                if(info.enable == '1'){
                  $("#follow_enb").prop("checked",true);
                } else {
                  $("#follow_enb").prop("checked",false);
                }
            }
        
            GetJCP({cmd: "presetcfg -act list",ParseJCP: function(preset){
                if(preset!='Error'){
                    const select = document.getElementById("preset");
                    // id=0 为关闭看守位不做隐藏处理
                    for(var i = 1; i < preset.length; i++) {
                        if(preset[i].enable === "0") {
                            const index = parseInt(preset[i].id);
                            if (!isNaN(index) && select.options[index]) {
                                select.options[index].style.display = 'none';
                            }
                        }
                    }
                }
            }});
        }});
    }
}

clickDrawPeople = function() {
    document.IPCamera.IPCSetHDMode(0, true);
    document.IPCamera.IPCSetHDInfo(0, "");
}

clickDelPeople = function() {
    document.IPCamera.IPCSetHDInfo(0, "");
    GetJCP({cmd:"humandetectcfg -act set -drag 0 -mbdesc " + empty, ParseJCP: function(result){
        var prompt = result == "Error" ? (IDC_DEL + IDC_FAIL) : (IDC_DEL + IDC_SUCCESS);
        alert(prompt);
    }})
    window.focus();
}

clickSavePeople = function() {
    var mbdesc = document.IPCamera.IPCGetHDInfo(0);
    var enb = $("#people_monitor_enb").is(":checked") == true ? 1 : 0;
    var screenenable = $("#people_blink_enb").is(":checked") == true ? 1 : 0;
    var centerenable = $("#people_center_enb").is(":checked") == true ? 1 : 0;
    var faceae = $("#face_illumination_enb").is(":checked") == true ? 1 : 0;
    var outdoor = $("#select_people_scene_mode").val();

    var str = "humandetectcfg -act set -timestrategy " + $("#people_monitor_time_protection").selectTime('getData');
    str += " -mbdesc " + mbdesc + " -enable " + enb + " -thresh " + $("#m_slider_span_body_distance").html();
    str += " -person_center " + centerenable;
    str += " -faceae " + faceae;
    str += " -screenenable " + screenenable;
    // str += " -humandistance " +  + $("#m_slider_span_body_distance").html();
    str += " -drag 0";
    str += " -outdoor " + outdoor;

    GetJCP({cmd:str , ParseJCP: function(result){
        var prompt = result == "Error" ? (IDC_SAVE + IDC_FAIL) : (IDC_SAVE + IDC_SUCCESS);
        alert(prompt);
        window.focus();
    }});
}

clickSaveCarDetect = function() {
    var mbdesc = document.IPCamera.IPCGetHDInfo(0);
    var enb = $("#car_detect_monitor_enb").is(":checked") == true ? 1 : 0;
    var screenenable = $("#car_detect_blink_enb").is(":checked") == true ? 1 : 0;
        
    var str = "cardetectcfg -act set -timestrategy " + $("#car_detect_monitor_time_protection").selectTime('getData');
    str += " -mbdesc " + mbdesc + " -enable " + enb + " -thresh " + $("#m_slider_span_car_detect_sensitivity").html();
    str += " -screenenable " + screenenable;

    GetJCP({cmd:str , ParseJCP: function(result){
        var prompt = result == "Error" ? (IDC_SAVE + IDC_FAIL) : (IDC_SAVE + IDC_SUCCESS);
        alert(prompt);
        window.focus();
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
        var prompt = result == "Error" ? (IDC_SAVE + IDC_FAIL) : (IDC_SAVE + IDC_SUCCESS);
        alert(prompt);
        window.focus();
    }});
  })

  $("#m_delect").click(function(){
    document.IPCamera.IPCSetMDInfo(0, 0, empty)
    GetJCP({cmd:"mdmbcfg -act set -mbdesc " + empty, ParseJCP: function(result){
        var prompt = result == "Error" ? (IDC_DEL + IDC_FAIL) : (IDC_DEL + IDC_SUCCESS);
        alert(prompt);
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
        
    $('#m_slider_area_monitor').slider({
        min: 1,
        max: 100,
        slide: (function(event, ui) {
            $("#m_slider_span_area_monitor").html(ui.value);
        })
    });
        
    $('#m_slider_cross_monitor').slider({
        min: 1,
        max: 100,
        slide: (function(event, ui) {
            $("#m_slider_span_cross_monitor").html(ui.value);
        })
    });

        
    // $('#m_slider_env_sensitivity').slider({
    // 	min: 1,
    // 	max: 100,
    // 	slide: (function(event, ui) {
    // 		$("#m_slider_span_env_sensitivity").html(ui.value);
    // 	})
    // });

        
    $('#m_slider_body_distance').slider({
        min: 1,
        max: 100,
        slide: (function(event, ui) {
            $("#m_slider_span_body_distance").html(ui.value);
            
            var str = "humandetectcfg -act set ";
            str += " -drag 1 ";
            str += " -thresh " +  ui.value;

            GetJCP({cmd:str , ParseJCP: function(result){}});

        })
    });

    $('#m_slider_car_detect_sensitivity').slider({
        min: 1,
        max: 100,
        slide: (function(event, ui) {
            $("#m_slider_span_car_detect_sensitivity").html(ui.value);
            
            var str = "cardetectcfg -act set ";
            str += " -drag 1 ";
            str += " -thresh " +  ui.value;

            GetJCP({cmd:str , ParseJCP: function(result){}});

        })
    });

    $("#cross_monitor_tab").hide();
    $("#area_monitor_tab").hide();
    $("#people_monitor_tab").hide();
    $("#car_detect_monitor_tab").hide();
}

_init_load = function(){
  // $("#motion_lab").html(IDC_MOTIONDETECT);
  // $("#motiontp_lab").html(IDC_TIME_PROTECTION);
  // $("#motion_tp_tip").html(IDC_TIME_PROTECTION);
  $("#m_select").html(IDC_SELECT_TABLE);
  $("#m_show").html(IDC_SHOW);
  $("#m_setting").html(IDC_SAVE)
  $("#m_delect").html(IDC_DEL)
  $("#m_save").html(IDC_SAVE);
  $("#ms_span").html("");
  $("#select_prompt").html("");
  // $("#motion_enb_lab").html(IDC_MD_ENABLE)
  $("#follow_enb_lab").html(IDC_MF_ENABLE);
  // $("#motion_time_protection").selectTime();
  $("#follow_time_protection").selectTime();
  $("#cross_monitor_time_protection").selectTime();
  $("#area_monitor_time_protection").selectTime();
  $("#people_monitor_time_protection").selectTime();
  $("#car_detect_monitor_time_protection").selectTime();
}

function FireChannelEvent(msg,chn, str){
  if (msg == 11) {
    IPCWndInit();
  } else if(msg == 12) {
    handleMotionDetect(str);
  }
}

function handleMotionDetect(alarm_msg){
  var msg_type = alarm_msg.split(";")[0].split("=")[1];
  switch(msg_type) {
    case "MOTIONDETECT":
        $("#mdTip").html(IDC_MOTIONDETECT_ALARM);
        window.setTimeout(function(){
          $("#mdTip").html('');
        }, 2000);
        break;
    case "VGLINEDETECT":
        $("#cross_tip").html(IDC_monitor_cross_alarm);
        window.setTimeout(function(){
          $("#cross_tip").html('');
        }, 2000);
        break;
    case "VGRECTDETECT":
        $("#area_tip").html(IDC_monitor_area_alarm);
        window.setTimeout(function(){
          $("#area_tip").html('');
        }, 2000);
        break;
    case "HUMANDETECT":
        $("#people_tip").html(IDC_HUMAN_DETECTION_ALARM);
        window.setTimeout(function(){
          $("#people_tip").html('');
        }, 2000);
        break;
    case "CARDETECT":
        $("#car_tip").html(IDC_CAR_DETECTION_ALARM);
        window.setTimeout(function(){
          $("#car_tip").html('');
        }, 2000);
        break;
    default:
        break;
  }

}

var g_curr_tab = "IDC_MF_ALARM";
function clickMD(){
    window.focus();
    if(g_curr_tab == "IDC_MENU_MOTION") {
        return;
    }

    if (g_curr_tab == "IDC_MOTION_FOLLOW" || g_curr_tab == "IDC_CROSS_ALARM" || g_curr_tab == "IDC_AREA_ALARM" || 
        g_curr_tab == "IDC_PEOPLE_ALARM" || g_curr_tab == "IDC_PEOPLE_CAR_DETECT") {
        $("#follow_monitor_tab").hide();
        $("#cross_monitor_tab").hide();
        $("#area_monitor_tab").hide();
        $("#people_monitor_tab").hide();
        $("#people_monitor_tab").hide();
        $("#car_detect_monitor_tab").hide();
        $("#m_btn_tab").show();
        g_curr_tab = "IDC_MENU_MOTION";

        document.IPCamera.IPCSetVGInfo(0, "");
        document.IPCamera.IPCSetVGMode(0, 0);

        IPCWndInit();
    } else {
        Set_cookie("alarm_last_menu", "IDC_MENU_MOTION");
        window.location.href = window.location.href;
    }
}

function clickMF(){
    window.focus();
    if(g_curr_tab == "IDC_MF_ALARM") {
        return;
    }

    if (g_curr_tab == "IDC_MENU_MOTION" || g_curr_tab == "IDC_CROSS_ALARM" || g_curr_tab == "IDC_AREA_ALARM" || 
        g_curr_tab == "IDC_PEOPLE_ALARM" || g_curr_tab == "IDC_PEOPLE_CAR_DETECT") {
        $("#m_btn_tab").hide();
        $("#cross_monitor_tab").hide();
        $("#area_monitor_tab").hide();
        $("#people_monitor_tab").hide();
        $("#car_detect_monitor_tab").hide();
        $("#follow_monitor_tab").show();
        g_curr_tab = "IDC_MF_ALARM";
        
        document.IPCamera.IPCSetMDInfo(0, 0,"");
        document.IPCamera.IPCSetHDInfo(0, "");
        document.IPCamera.IPCSetVGInfo(0, "");

        IPCWndInit();
    } else {
        Set_cookie("alarm_last_menu", "IDC_MF_ALARM");
        window.location.href = window.location.href;
    }
}

function clickPeopleAlarm(){
    window.focus();
    if(g_curr_tab == "IDC_PEOPLE_ALARM") {
        return;
    }

    if (g_curr_tab == "IDC_MF_ALARM" || g_curr_tab == "IDC_CROSS_ALARM" || g_curr_tab == "IDC_AREA_ALARM" || 
        g_curr_tab == "IDC_MENU_MOTION" || g_curr_tab == "IDC_PEOPLE_CAR_DETECT") {
        $("#follow_monitor_tab").hide();
        $("#cross_monitor_tab").hide();
        $("#area_monitor_tab").hide();
        $("#m_btn_tab").hide();
        $("#car_detect_monitor_tab").hide();
        $("#people_monitor_tab").show();
        g_curr_tab = "IDC_PEOPLE_ALARM";

        document.IPCamera.IPCSetVGInfo(0, "");
        document.IPCamera.IPCSetMDInfo(0, 0,"");

        IPCWndInit();
    } else {
        Set_cookie("alarm_last_menu", "IDC_PEOPLE_ALARM");
        window.location.href = window.location.href;
    }
}



function clickCrossAlarm() {
    window.focus();
    if(g_curr_tab == "IDC_CROSS_ALARM") {
        return;
    }

    if (g_curr_tab == "IDC_MF_ALARM" || g_curr_tab == "IDC_MENU_MOTION" || g_curr_tab == "IDC_AREA_ALARM" || 
        g_curr_tab == "IDC_PEOPLE_ALARM" || g_curr_tab == "IDC_PEOPLE_CAR_DETECT") {
        $("#m_btn_tab").hide();
        $("#follow_monitor_tab").hide();
        $("#area_monitor_tab").hide();
        $("#people_monitor_tab").hide();
        $("#car_detect_monitor_tab").hide();
        $("#cross_monitor_tab").show();
        g_curr_tab = "IDC_CROSS_ALARM";

        document.IPCamera.IPCSetMDInfo(0, 0,"");
        document.IPCamera.IPCSetHDInfo(0, "");
        document.IPCamera.IPCSetVGInfo(0, "");

        IPCWndInit();
    } else {
        Set_cookie("alarm_last_menu", "IDC_CROSS_ALARM");
        window.location.href = window.location.href;
    }
}

function clickPeopleCarDetect(){
    window.focus();
    if(g_curr_tab == "IDC_PEOPLE_CAR_DETECT") {
        return;
    }

    if (g_curr_tab == "IDC_MF_ALARM" || g_curr_tab == "IDC_CROSS_ALARM" || g_curr_tab == "IDC_AREA_ALARM" || 
        g_curr_tab == "IDC_MENU_MOTION" || g_curr_tab == "IDC_PEOPLE_ALARM") {
        $("#follow_monitor_tab").hide();
        $("#cross_monitor_tab").hide();
        $("#area_monitor_tab").hide();
        $("#m_btn_tab").hide();
        $("#people_monitor_tab").hide();
        $("#car_detect_monitor_tab").show();
        g_curr_tab = "IDC_PEOPLE_CAR_DETECT";

        document.IPCamera.IPCSetVGInfo(0, "");
        document.IPCamera.IPCSetMDInfo(0, 0,"");

        IPCWndInit();
    } else {
        Set_cookie("alarm_last_menu", "IDC_PEOPLE_CAR_DETECT");
        window.location.href = window.location.href;
    }
}

function getOCXShowXPosition(x) {
    var _x = parseInt(x * veWidth / wndWidth);
    return _x > 1920 ? 1920 : Math.abs(_x);
}

function setOCXShowXPosition(x) {
    return parseInt(x * wndWidth / veWidth);
}

function getOCXShowYPosition(y) {
    var _y = parseInt(y * veHeight / wndHeight);
    return _y > 1080 ? 1080 : Math.abs(_y);
}

function setOCXShowYPosition(y) {
    return parseInt(y * wndHeight / veHeight);
}

function clickAreaAlarm() {
    window.focus();
    if(g_curr_tab == "IDC_AREA_ALARM") {
        return;
    }

    if (g_curr_tab == "IDC_MF_ALARM" || g_curr_tab == "IDC_MENU_MOTION" || g_curr_tab == "IDC_CROSS_ALARM" || 
        g_curr_tab == "IDC_PEOPLE_ALARM" || g_curr_tab == "IDC_PEOPLE_CAR_DETECT") {
        $("#m_btn_tab").hide();
        $("#follow_monitor_tab").hide();
        $("#cross_monitor_tab").hide();
        $("#people_monitor_tab").hide();
        $("#car_detect_monitor_tab").hide();
        $("#area_monitor_tab").show();
        g_curr_tab = "IDC_AREA_ALARM";

        document.IPCamera.IPCSetMDInfo(0, 0,"");
        document.IPCamera.IPCSetHDInfo(0, "");
        document.IPCamera.IPCSetVGInfo(0, "");

        IPCWndInit();
    } else {
        Set_cookie("alarm_last_menu", "IDC_AREA_ALARM");
        window.location.href = window.location.href;
    }
    
}

function clickDrawLine() {
    window.focus();
    document.IPCamera.IPCSetVGMode(0, 1);
    document.IPCamera.IPCSetVGInfo(0, "");
    old_dir = $("#select_cross_direction").val();
    document.IPCamera.IPCSetVGLineDir(0, parseInt(old_dir));
}

function clickSaveFollow() {
    window.focus();
    
    var preset = $("#preset").val();
    var remainedtime = $("#remainedtime").val();
    var enb = $("#follow_enb").is(":checked") == true ? 1 : 0;
    
    var str = "followcfg -act set -timestrategy " + $("#follow_time_protection").selectTime('getData');
    str += " -enable " + enb + " -preset " + preset + " -idle " + remainedtime;
    if(preset < 0 || preset > 6){
                alert(IDC_SAVE + IDC_FAIL);
                return;
    }
    
    if(remainedtime < 3 || remainedtime > 255){
                alert(IDC_SAVE + IDC_FAIL);
                return;
    }
     
    GetJCP({cmd:str , ParseJCP: function(result){
        var prompt = result == "Error" ? (IDC_SAVE + IDC_FAIL) : (IDC_SAVE + IDC_SUCCESS);
        alert(prompt);
        window.focus();
    }});
}

function clickSaveLine() {
    window.focus();
     
    var enb = $("#cross_monitor_enb").is(":checked") == true ? 1 : 0;
    var blink_enb = $("#cross_blink_enb").is(":checked") == true ? 1 : 0;

    var str = "vgline -act set -timestrategy " + $("#cross_monitor_time_protection").selectTime('getData');
    str += " -enable " + enb + " -thresh " + $("#m_slider_span_cross_monitor").html();
    str += " -indoor " + $("#select_cross_scene_mode").val();
    str += " -dir " + $("#select_cross_direction").val();
    str += " -blink " + blink_enb;
    
    do
    {
        var _ocx_info = document.IPCamera.IPCGetVGInfo(0);

        if(_ocx_info == '' || _ocx_info.indexOf("x0") < 0) {
            str += " -x0 0";
            str += " -y0 0";
            str += " -x1 0";
            str += " -y1 0";
            str += " -dx0 0";
            str += " -dy0 0";
            str += " -dx1 0";
            str += " -dy1 0";
        } else {
            var _map = parse_jcp_content(_ocx_info, {spliter1: '=', spliter2: ','});

            str += " -x0 " + getOCXShowXPosition(_map.x0);
            str += " -y0 " + getOCXShowYPosition(_map.y0);
            str += " -x1 " + getOCXShowXPosition(_map.x1);
            str += " -y1 " + getOCXShowYPosition(_map.y1);
            str += " -dx0 " + getOCXShowXPosition(_map.x2);
            str += " -dy0 " + getOCXShowYPosition(_map.y2);
            str += " -dx1 " + getOCXShowXPosition(_map.x3);
            str += " -dy1 " + getOCXShowYPosition(_map.y3);
        }
    }
    while (0);
    
    
    GetJCP({cmd:str , ParseJCP: function(result){
        var prompt = result == "Error" ? (IDC_SAVE + IDC_FAIL) : (IDC_SAVE + IDC_SUCCESS);
        alert(prompt);
        window.focus();
    }});

}

function clickDelLine() {
    window.focus();
    document.IPCamera.IPCSetVGInfo(0, "");
    var str = "vgline -act set";
    str += " -x0 0";
    str += " -y0 0";
    str += " -x1 0";
    str += " -y1 0";
    str += " -dx0 0";
    str += " -dy0 0";
    str += " -dx1 0";
    str += " -dy1 0";
    GetJCP({cmd:str, ParseJCP: function(result){
        var prompt = result == "Error" ? (IDC_DEL + IDC_FAIL) : (IDC_DEL + IDC_SUCCESS);
        alert(prompt);
    }})
}

function clickDrawArea() {
    window.focus();
    document.IPCamera.IPCSetVGMode(0, 2);
    document.IPCamera.IPCSetVGInfo(0, "");
}

function clickSaveArea() {
    window.focus();
     
    var enb = $("#area_monitor_enb").is(":checked") == true ? 1 : 0
    var blink_enb = $("#area_blink_enb").is(":checked") == true ? 1 : 0;

    var str = "vgrect -act set -timestrategy " + $("#area_monitor_time_protection").selectTime('getData');
    str += " -enable " + enb + " -thresh " + $("#m_slider_span_area_monitor").html();
    str += " -indoor " + $("#select_area_scene_mode").val();
    str += " -dir " + $("#select_area_direction").val();
    str += " -blink " + blink_enb;

    do
    {
        var _ocx_info = document.IPCamera.IPCGetVGInfo(0);

        if(_ocx_info == '' || _ocx_info.indexOf("x0") < 0) {
            str += " -x0 0";
            str += " -y0 0";
            str += " -x1 0";
            str += " -y1 0";
            str += " -x2 0";
            str += " -y2 0";
            str += " -x3 0";
            str += " -y3 0";
        } else {
            var _map = parse_jcp_content(_ocx_info, {spliter1: '=', spliter2: ','});

            str += " -x0 " + getOCXShowXPosition(_map.x0);
            str += " -y0 " + getOCXShowYPosition(_map.y0);
            str += " -x1 " + getOCXShowXPosition(_map.x1);
            str += " -y1 " + getOCXShowYPosition(_map.y1);
            str += " -x2 " + getOCXShowXPosition(_map.x2);
            str += " -y2 " + getOCXShowYPosition(_map.y2);
            str += " -x3 " + getOCXShowXPosition(_map.x3);
            str += " -y3 " + getOCXShowYPosition(_map.y3);
        }
    }
    while (0);

 
    GetJCP({cmd:str , ParseJCP: function(result){
        var prompt = result == "Error" ? (IDC_SAVE + IDC_FAIL) : (IDC_SAVE + IDC_SUCCESS);
        alert(prompt);
        window.focus();
    }});
}

function clickDelArea() {
    window.focus();
    document.IPCamera.IPCSetVGInfo(0, "");
    var str = "vgrect -act set";
    str += " -x0 0";
    str += " -y0 0";
    str += " -x1 0";
    str += " -y1 0";
    str += " -x2 0";
    str += " -y2 0";
    str += " -x3 0";
    str += " -y3 0";
    GetJCP({cmd:str, ParseJCP: function(result){
        var prompt = result == "Error" ? (IDC_DEL + IDC_FAIL) : (IDC_DEL + IDC_SUCCESS);
        alert(prompt);
    }})
}


function changeCrossDirection() {
    var r = $("#select_cross_direction").val();

    var _ocx_info = document.IPCamera.IPCGetVGInfo(0);

    if (_ocx_info == '' || _ocx_info.indexOf("x0") < 0) {
        return;
    }

    var info = parse_jcp_content(_ocx_info, {spliter1: '=', spliter2: ','});

    var msg = "";

    if (r == 0 || r == 1) {
        if (old_dir == 2) {
            var x3 = parseInt(info.x0) + parseInt(info.x1) - parseInt(info.x2);
            var y3 = parseInt(info.y0) + parseInt(info.y1) - parseInt(info.y2);

            msg = "x0=" + info.x0 + ",y0=" + info.y0 + ",";
            msg += "x1=" + info.x1 + ",y1=" + info.y1 + ",";

            if(r == 0) {
                msg += "x2=" + info.x2 + ",y2=" + info.y2 + ",";
                msg += "x3=" + x3 + ",y3=" + y3 + ",";
            } else {
                msg += "x2=" + x3 + ",y2=" + y3 + ",";
                msg += "x3=" + info.x2 + ",y3=" + info.y2 + ",";
            }
        } else {
            msg = "x0=" + info.x0 + ",y0=" + info.y0 + ",";
            msg += "x1=" + info.x1 + ",y1=" + info.y1 + ",";
            msg += "x2=" + info.x3 + ",y2=" + info.y3 + ",";
            msg += "x3=" + info.x2 + ",y3=" + info.y2 + ",";
        }
    } else {
        msg = "x0=" + info.x0 + ",y0=" + info.y0 + ",";
        msg += "x1=" + info.x1 + ",y1=" + info.y1 + ",";
        msg += "x2=" + info.x2 + ",y2=" + info.y2 + ",";
        msg += "x3=" + info.x2 + ",y3=" + info.y2 + ",";
    }

    old_dir = r;

    
    document.IPCamera.IPCSetVGMode(0, 1);
    document.IPCamera.IPCSetVGInfo(0, "");
    document.IPCamera.IPCSetVGInfo(0, msg);

}


var POINT_2_POINT_INTERVAL = 10;

function Point(x, y) {
    this.x = x;
    this.y = y;
}

/**
 * 0:A->B,1:B->A, 2:A<->B
 */
function JudgeDirection(p0, p1, p2, p3) {
    var res = -1;
    do
    {
        if (4 > arguments.length)
        {
            break;
        }

        // 计算方向
        if (IsP2PNear(p2, p3, POINT_2_POINT_INTERVAL) == true)
        {
            res = 2;
            break;
        }

        var fDP0P1 = Point2Degree(p0, p1);
        var fDP0D0 = Point2Degree(p0, p2);
        if (fDP0D0 < fDP0P1)
        {
            res = 0;
        }
        else
        {
            res = 1;
        }

        if (fDP0D0- fDP0P1 > Math.PI / 2)
        {
            res = 0;
        }
        else if(fDP0P1-fDP0D0 > Math.PI / 2)
        {
            res = 1;
        }

    } while (0);

    return res;
}	

function IsP2PNear(pntA, pntB, iInterval) {
    var fP2P = Math.sqrt(Math.pow(parseInt(pntA.x) - parseInt(pntB.x), 2) + Math.pow(parseInt(pntA.y) - parseInt(pntB.y), 2));
    return fP2P <= iInterval ? true : false;
}

function Point2Degree(pntA, pntB) {
    if (0.00001 >= Math.abs(parseInt(pntA.x) - parseInt(pntB.x)))
    {// atan无穷大
        if (parseInt(pntA.y) < parseInt(pntB.y))
        {
            return Math.PI / 2;
        }
        else
        {
            return Math.PI / 2 * 3;
        }
    }
    else if (0.00001 >= Math.abs(parseInt(pntA.y) - parseInt(pntB.y)))
    {// atan为零
        if (parseInt(pntA.x) < parseInt(pntB.x))
        {
            return 0;
        }
        else
        {
            return Math.PI;
        }
    }
    
    var fDegree = Math.atan((parseInt(pntB.y) - parseInt(pntA.y)) * 1.0 / (parseInt(pntB.x) - parseInt(pntA.x)));
    if (parseInt(pntA.x) > parseInt(pntB.x))
    {// 二三象限
        fDegree += Math.PI;
    }
    fDegree = Math.PI * 2 <= fDegree ? fDegree - Math.PI * 2 : (0 > fDegree ? fDegree + Math.PI * 2 : fDegree);

    return fDegree;
}
