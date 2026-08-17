var initVideoShow=0;
var wndWidth = 620; //视频宽度
var wndHeight = 470; //视频高度
var veWidth=1920, veHeight=1080; //用插件获取到的尺寸
var has_dome = $.cookie("has_dome");
var shelter = $.cookie("shelter");
var masterCfgArr; 
var slaveCfgArr; 
var x264Rate;
var audioCfgArr;

$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf || typeof(lf) =='undefined' || 0 > parseInt(lf))
    {
          parent.location.href = "/login.asp";
    }else{
        if(parseInt(GetCookieByKey("has_audio")) == 1){
            $("#liAudioSet").show();
        } 
        
        initNightTime();

        if(has_dome == 1){ //球机
            $("#liInfraredSet").hide(); //隐藏红外设置
            $("#liColorParamSet").hide(); //隐藏颜色参数设置
            $("#liAdvanceSet").hide(); //隐藏高级设置tab
            $("#tabs-4").hide();

            if(shelter == 0){
                $("#liPrivacyMask").hide();
            }else{
                $("#tbDome").show();
                $("#tbQJ").hide();
                ptz_click_bind();
            }
        }else{
            $("#subtabs_color-1").show(); //默认隐藏
            $("#subtabs_color-3").show(); //默认隐藏

            $("#tbDome").hide();
            $("#tbQJ").show();
        }
        
        $("#tabs").tabs();
        
        $("select").bind("change",function(){
             window.focus();
        }); 

        //主从码流显示
        showStream();

        //视频参数
        $("#tab_videoparams").click(function(){
            $("#subtabs_mask").hide();//视频遮挡页面隐藏
            $("#subtabs_color").hide();//图像设定tab隐藏
            $("#subtabs_roisetting").hide();
            $("#subtabs_param").tabs();
            $("#subtabs_param").show();//基础字幕，扩展字幕等tabs页面显示
            initShowVideoInfo();//显示视频
            showVideoPAramInfo();//显示基础字幕，扩展字幕等视频参数信息
            window.focus();
        });

        //视频遮挡
        $("#tab_videomask").click(function(){
            //基础字幕，扩展字幕等tabs页面隐藏
            $("#subtabs_param").hide();
            $("#subtabs_color").hide();//图像设定tab隐藏
            $("#subtabs_roisetting").hide();
            $("#subtabs_mask").tabs();
            $("#subtabs_mask").show();//视频遮挡页面显示
            initShowVideoInfo();//显示视频
            showVideoMask();//展示视频遮挡信息
            window.focus();
        });

        //图像设定
        $("#tab_colorsetting").click(function(){
            $("#subtabs_mask").hide();
            $("#subtabs_param").hide();
            $("#subtabs_roisetting").hide();
            $("#subtabs_color").tabs();
            $("#subtabs_color").show();
            initShowVideoInfo();
            initVideoSetting();//图像设置
            window.focus();
        });

        //ROI设置
        $("#tab_roisetting").click(function(){
            $("#subtabs_mask").hide();
            $("#subtabs_param").hide();
            $("#subtabs_color").hide();
            $("#subtabs_roisetting").tabs();
            $("#subtabs_roisetting").show();
            initShowVideoInfo();
            showRoiInfo();
            window.focus();
        });
        

        //扩展配置
        $("#tab_entendconfig").click(function(){
            if(initVideoShow>0){
                initVideoShow=0;
                document.IPCamera.IPCStopPreview(0);
                $("#objects").html('');
            }
            show3dNoise();//显示3d数字降噪
            showEncodeSetting();//编码设置
            //showHikvisonNVR();
            window.focus();
        });

        //音频设置
        $("#tab_audioset").click(function(){
            if(initVideoShow>0){
                initVideoShow=0;
                document.IPCamera.IPCStopPreview(0);
                $("#objects").html('');
            }
            showAudioInfo();
        });

        //灯光设置
        $("#tab_infraredsetting").click(function(){
            if(initVideoShow>0){
                initVideoShow=0;
                document.IPCamera.IPCStopPreview(0);
                $("#objects").html('');
            }
            showInfraredSet();
        });

        //视频通道
        $("#tab_videochannel").click(function(){
            if(initVideoShow>0){
                initVideoShow=0;
                document.IPCamera.IPCStopPreview(0);
                $("#objects").html('');
            }
            showStream();
            window.focus();
        });
    }

});

function zeroAdd(num){
    if(num < 10){
        return "0" + num;
    }
    return num;
}

function initNightTime(){
    var _html_night_time1 = '';
    var _html_night_time2 = '';
    var _html_night_time3 = '';
    var _html_night_time4 = '';
    var _html_timing_time1 = '';
    var _html_timing_time2 = '';
    var _html_timing_time3 = '';
    var _html_timing_time4 = '';
    _html_night_time1 += '<select id="night_time1" style="width:50px;margin-left:0px;" class="sysinput2">';
    _html_night_time2 += '<select id="night_time2" style="width:50px;margin-left:0px;" class="sysinput2">';
    _html_night_time3+= '<select id="night_time3" style="width:50px;margin-left:0px;" class="sysinput2">';
    _html_night_time4 += '<select id="night_time4" style="width:50px;margin-left:0px;" class="sysinput2">';
    _html_timing_time1 += '<select id="timing_time1" style="width:50px;margin-left:0px;" class="sysinput2">';
    _html_timing_time2 += '<select id="timing_time2" style="width:50px;margin-left:0px;" class="sysinput2">';
    _html_timing_time3 += '<select id="timing_time3" style="width:50px;margin-left:0px;" class="sysinput2">';
    _html_timing_time4 += '<select id="timing_time4" style="width:50px;margin-left:0px;" class="sysinput2">';
    for(var i=0;i<24;i++){
         _html_night_time1 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
         _html_night_time3 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
         _html_timing_time1 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
         _html_timing_time3 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
    }
    
    for(var i=0;i<60;i++){
         _html_night_time2 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
         _html_night_time4 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
         _html_timing_time2 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
         _html_timing_time4 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
    }
    
    _html_night_time1 += '</select>';
    _html_night_time2 += '</select>';
    _html_night_time3 += '</select>';
    _html_night_time4 += '</select>';
    
    _html_timing_time1 += '</select>';
    _html_timing_time2 += '</select>';
    _html_timing_time3 += '</select>';
    _html_timing_time4 += '</select>';
    
    $("#tdNightTime").html(_html_night_time1 + ":" + _html_night_time2 + "&nbsp;&nbsp;~&nbsp;&nbsp;" + _html_night_time3 + ":" + _html_night_time4);
    
    $("#tdTimingTime").html(_html_timing_time1 + ":" + _html_timing_time2 + "&nbsp;&nbsp;~&nbsp;&nbsp;" + _html_timing_time3 + ":" + _html_timing_time4);
}

//切换tab时清除
function cleanVideoParam(){
    document.IPCamera.IPCShowMDAreaEx(0, false);
    document.IPCamera.IPCShowMDAreaEx(1, false);
    document.IPCamera.IPCShowMDAreaEx(2, false);
    document.IPCamera.IPCShowMDAreaEx(3, false);
}

//显示基础字幕，扩展字幕等视频参数信息
function showVideoPAramInfo(){
    $("#subtabs_param").tabs(); //显示二级tabs
    initVideoParams();//初始化视频参数信息
}

//初始化视频信息
function initShowVideoInfo(){
    if(initVideoShow==0){
        initVideoShow++
        showVideo();//展示视频
    }
    else{
        cleanVideoParam();
    }
}

var infraredSensitivity;
function showInfraredSet(){
    infraredSensitivity = $("#sensitivity").slider({
        change: function (event, ui)
        {
            $("#tdSensitivity").html(ui.value);
        },
        min:1,
        max:100
    });
    
    GetJCP({cmd: "lightextcfg -act list ", ParseJCP: function(jcpObj){
        $("#lightmode").val(jcpObj.devtype);
        $("#lightonoff").val(jcpObj.irswitchmode);

        var _blacktime = jcpObj.blacktime;
        $("#night_time1").val(jcpObj.beginhour);
        $("#night_time2").val(jcpObj.beginmin);
        $("#night_time3").val(jcpObj.endhour);
        $("#night_time4").val(jcpObj.endmin);

        $("#white_reverse").val(jcpObj.whitectrl);
        $("#infrared_reverse").val(jcpObj.irledctrl);

        //feature =2, 4, 只允许设置黑白模式。
        //feature =1, 3, 都允许
        var feature = GetCookieByKey("feature");
        if(feature != 'undefined' && (feature == 2 || feature == 4) ) {
            $("#shinemode option[value='0']").remove(); 
            $("#shinemode option[value='2']").remove(); 
        }

        if (jcpObj.lightboard == 0) {
            $("#tr_dbl").hide();
            $("#tr_white").hide();
            $("#tr_ir").show();
        } else if (jcpObj.lightboard == 1) {
            $("#tr_dbl").hide();
            $("#tr_white").show();
            $("#tr_ir").hide();
        } else {
            $("#tr_dbl").show();
            $("#tr_white").hide();
            $("#tr_ir").hide();
        }

        changeSwitchMode();

        $("#sensType").html(jcpObj.lampmode==0?IDC_HARD_SENS:IDC_SOFT_SENS);
        $("#sensRead").html(jcpObj.irledmode==0?IDC_LOW_SENS:IDC_HIGH_SENS);
    }});
}

function changeShineMode() {
    var _v = $("#shinemode").val();
    if(_v == 0) { //全彩模式
        $("#tr_timing_time").show();
        $("#tr_white_light_control").show();
        $("#tr_shine_time").hide();
        $("#tr_sensitivity").hide();
    }else {
        $("#tr_timing_time").hide();
        if(_v == 1) { //黑白模式
            $("#tr_shine_time").hide();
            $("#tr_white_light_control").hide();
            $("#tr_sensitivity").show();
        } else { //智能模式
            $("#tr_shine_time").show();
            $("#tr_white_light_control").show();
            $("#tr_sensitivity").hide();
        }

    }
}

function changeAutolightEn(){
    var autolighten = $("#pwm_autolight_en").is(":checked") == true ? 1 : 0;
    if(autolighten == 1) {
        $("#tr_light_pwm_dimmer").hide();
    } else {
        $("#tr_light_pwm_dimmer").hide();
    }
}

function changeLightOnOff(){
    var _v = parseInt($("#lightonoff").val());
    switch(_v) {
        case 2:  // 自动
            $("#tr_night_time").hide();
            break;
        case 3:  // 定时
            $("#tr_night_time").show();
            break;
        default:
            break;
    }
}

function changeLightMode(){
    var _v = parseInt($("#lightonoff").val());
    switch(_v) {
        case 2:  // 自动
            $("#tr_night_time").hide();
            $("#tr_light_abjust").hide();
            $("#tr_light_threshold").show();
            $("#slide_light_level").hide();
            break;
        case 3:  // 定时
            $("#tr_night_time").show();
            $("#tr_light_abjust").show();
            $("#tr_light_threshold").hide();
            $("#slide_light_level").hide();
            break;
        case 4:  // 常关
            $("#tr_night_time").hide();
            $("#tr_light_abjust").hide();
            $("#tr_light_threshold").hide();
            $("#slide_light_level").hide();
            break;
        default:
            break;
    }
}

function changeSwitchMode(){
    var _v = parseInt($("#lightmode").val());
    switch(_v) {
        case 0: //红外模式
            $("#tr_light_onoff").show();
            $("#tr_white_light_bright").hide();
            $("#tr_sensitivity").hide();
            $("#tr_shine_mode").hide();
            $("#tr_shine_time").hide();
            $("#tr_white_light_control").hide();
            $("#tr_inf_light_control").hide();
            changeLightOnOff()
            $("#tr_light_pwm").hide();
            break;
        case 1: //全彩模式
            $("#tr_light_onoff").show();
            $("#tr_white_light_bright").hide();
            $("#tr_sensitivity").hide();
            $("#tr_shine_mode").hide();
            $("#tr_shine_time").hide();
            $("#tr_white_light_control").hide();
            $("#tr_inf_light_control").hide();
            changeLightOnOff()
            $("#tr_light_pwm").hide();
            break;
        case 2://智能双光
            $("#tr_light_onoff").show();
            $("#tr_white_light_bright").hide();
            $("#tr_sensitivity").hide();
            $("#tr_shine_mode").hide();
            $("#tr_shine_time").hide();
            $("#tr_white_light_control").hide();
            $("#tr_inf_light_control").hide();
            changeLightOnOff()
            $("#tr_light_pwm").hide();
            break;
        default:
            break;
    }

}

var lightgradetype,lightgradetypeVal=0; 
function initlightgradetype(jcpObj){
    try
    {
        lightgradetype = $("#lightgradetype").slider({
            slide: function (event, ui)
            {
                $("#tdlightgradetype").html(ui.value);
            },
            stop: function (event, ui)
            {
                lightgradetypeVal = ui.value;
            },
            min:1,
            max:100
        });

        $("#lightgradetypeVal").bind('slide', function(event, ui){
            $("#tdlightgradetype").html(ui.value);  
        });

        lightgradetypeVal = parseInt(jcpObj.lightgrade);
        lightgradetype.slider("value", lightgradetypeVal);
        $("#tdlightgradetype").html(jcpObj.lightgrade);
    }catch(e){}
}

function SaveInfraredSet(){
    /*if($("#night_time1").val()*60 + $("#night_time2").val()*1 > $("#night_time3").val()*60 +$("#night_time4").val()*1) {
        alert(IDC_PLAYBACK_TIME);
        window.focus();
        return;
    }*/
    var shinetime = $("#shinetime").val();
    if(shinetime < 10){
        $("#shinetime").val(10);
        shinetime = 10;
    }else if(shinetime > 60){
        $("#shinetime").val(60);
        shinetime = 60;
    }

    var _cmd = "lightextcfg -act set -devtype " + $("#lightmode").val() + " -irswitchmode " + $("#lightonoff").val() + " -beginhour " + $("#night_time1").val() + " -beginmin " + $("#night_time2").val() + " -endhour " + $("#night_time3").val() + " -endmin " + $("#night_time4").val();
            
    _cmd += " -whitectrl " + $("#white_reverse").val() + " -irledctrl " + $("#infrared_reverse").val();

    GetJCPList({cmd: _cmd }); 

    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
}

var nightluma, bright, contrast, saturation, 
    gain, contrastAgain, highLightSuppress,
    sharpness,lowlightenhance ;
var g_hdr = 0; //宽动态
//图像设定初始化
function initVideoSetting(){
    try
    {
        if(has_dome != 1){
            nightluma = $("#nightluma").slider({
                change: function (event, ui)
                {
                    $("#tdNightluma").html(ui.value);
                },
                stop: function (event, ui)
                {
                    GetJCP({cmd: "vicfg -act set  -nightluma " + ui.value});
                },
                min:1,
                max:100
            });
            $("#nightluma").bind('slide', function(event, ui){
                $("#tdNightluma").html(ui.value);  
            });

            bright = $("#bright").slider({
                change: function (event, ui)
                {
                    $("#tdBright").html(ui.value);
                },
                stop: function (event, ui)
                {
                    GetJCP({cmd: "vicfg -act set  -bright " + ui.value});
                },
                min:1,
                max:255
            });
            $("#bright").bind('slide', function(event, ui){
                $("#tdBright").html(ui.value);  
            });
           
            contrast = $("#contrast").slider({
                change: function (event, ui)
                {
                    $("#tdContrast").html(ui.value);
                },
                stop: function (event, ui)
                {
                    GetJCP({cmd: "vicfg -act set  -contrast " + ui.value});
                },
                min:1,
                max:255
            });
            $("#contrast").bind('slide', function(event, ui){
                $("#tdContrast").html(ui.value);  
            });

            /*contrastAgain = $("#contrastAgain").slider({
                change: function (event, ui)
                {
                    $("#tdContrastAgain").html(ui.value);
                },
                stop: function (event, ui)
                {
                    GetJCP({cmd: "vicfg -act set -stren 1 -brightlevel " + ui.value});
                },
                min:0,
                max:6
            });
            $("#contrastAgain").bind('slide', function(event, ui){
                $("#tdContrastAgain").html(ui.value);  
            });*/

            saturation = $("#saturation").slider({
                change: function (event, ui)
                {
                    $("#tdSaturation").html(ui.value);
                },
                stop: function (event, ui)
                {
                    GetJCP({cmd: "vicfg -act set  -saturation " + ui.value});
                },
                min:1,
                max:255
            });
            $("#saturation").bind('slide', function(event, ui){
                $("#tdSaturation").html(ui.value);  
            });

            sharpness = $("#sharpness").slider({
                change: function (event, ui)
                {
                    $("#tdSharpness").html(ui.value);
                },
                stop: function (event, ui)
                {
                    GetJCP({cmd: "vicfg -act set -sharpness " + ui.value});
                },
                min:1,
                max:255
            });
            $("#sharpness").bind('slide', function(event, ui){
                $("#tdSharpness").html(ui.value);  
            });

            gain = $("#gain").slider({
                change: function (event, ui)
                {
                    $("#tdGain").html(ui.value);
                },
                stop: function (event, ui)
                {
                    GetJCP({cmd: "vicfg -act set  -gain " + ui.value});
                },
                min:1,
                max:255
            });
            $("#gain").bind('slide', function(event, ui){
                $("#tdGain").html(ui.value);  
            });

            highLightSuppress = $("#highLightSuppress").slider({
                change: function (event, ui)
                {
                    $("#tdHighLightSuppress").html(ui.value);
                },
                stop: function (event, ui)
                {
                    GetJCP({cmd: "vicfg -act set  -suppress " + ui.value});
                },
                min:0,
                max:100
            });

            lowlightenhance = $("#lowlightenhance").slider({
                change: function (event, ui)
                {
                    $("#tdlowlightenhance").html(ui.value);
                },
                stop: function (event, ui)
                {
                   // GetJCP({cmd: "vicfg -act set  -lowlightenhance " + ui.value});
                },
                min:0,
                max:100
            });

            $("#highLightSuppress").bind('slide', function(event, ui){
                $("#tdHighLightSuppress").html(ui.value);  
            });

            GetJCP({cmd: "vicfg -act list ", ParseJCP: ParseVICfg});

        }else{
            $("#liVideoControl").click();
        }
        GetJCP({cmd: "aeawbblccfg -act list", ParseJCP: Parseaeawb});
        //GetJCP({cmd: "lenscs -act list", ParseJCP: ParseShadowCorrection});
    }
    catch(E){return E;}
}

//初始化图像设定-颜色参数信息
function ParseVICfg(jcpstr)
{
    try
    {
        nightluma.slider("value", parseInt(jcpstr.nightluma));
        bright.slider("value", parseInt(jcpstr.bright));
        contrast.slider("value", parseInt(jcpstr.contrast));
        saturation.slider("value", parseInt(jcpstr.saturation));
        sharpness.slider("value", parseInt(jcpstr.sharpness));
        gain.slider("value", parseInt(jcpstr.gain));
        highLightSuppress.slider("value", parseInt(jcpstr.suppress));
        /*contrastAgain.slider("value", parseInt(jcpstr.brightlevel));
        if(parseInt(jcpstr.stren) == 1){
            $("#cbContrastAgain").prop("checked",true);
        }else{
            $("#cbContrastAgain").prop("checked",false);
        }*/
        $("input[name='lampchk'][value=" + parseInt(jcpstr.lampfrequency) + "]").attr("checked",true);
        //checkContrastAgain();
    }
    catch(e){}
}

function checkContrastAgain(type){
    var check = $("#cbContrastAgain").is(":checked");
    if(check){
        $("#trBright").hide();
        $("#trContrast").hide();
        $("#trContrastAgain").show();
    }else{
        $("#trBright").show();
        $("#trContrast").show();
        $("#trContrastAgain").hide();
    }
    if(typeof type != 'undefined' && type == 1){
       var str = "vicfg -act set -stren ";
       str += check==true?1:0;
       str += " -brightlevel " + parseInt($("#tdContrastAgain").text());
       GetJCP({cmd: str});
   }
}

//初始化图像设定-高级设置
function  Parseaeawb(jcpGet)
{
    try
    {
        $("#AESelect").val(jcpGet.aeCtrlMode);
        $("#AWBSelect").val(jcpGet.awbCtrlMode);
        lowlightenhance.slider("value", parseInt(jcpGet.lowlightenhance));
        $("#tdlowlightenhance").html(jcpGet.lowlightenhance);

        if(7 == jcpGet.awbCtrlMode)
        {
            $("#redgain").attr("disabled",false);
            $("#blurgain").attr("disabled",false);
        }else{
            $("#redgain").attr("disabled",true);
            $("#blurgain").attr("disabled",true);
        }

        $("#redgain").val(jcpGet.redGain);
        $("#blurgain").val(jcpGet.blueGain);
        $("#nightfacemode").val(jcpGet.nightfacemode);

    }
    catch(e){ return e; }
}

//初始化阴影矫正
/*function ParseShadowCorrection(jcpGet){
    try{ 
        $("#shadowCorrection").prop("checked",jcpGet.enable==1?true:false);
    }catch(e){ return e; }
}*/

function changeAWBSelect(){
    var awb = $("#AWBSelect").val();
    if(7 == awb)
    {
        $("#redgain").attr("disabled",false);
        $("#blurgain").attr("disabled",false);
    }
    else
    {
        $("#redgain").attr("disabled",true);
        $("#blurgain").attr("disabled",true);

    }
}

function redgainBlur(){
    var redgains = $("#redgain").val();
    if(isNaN(redgains) || redgains < 1 || redgains > 255)
    {
        alert(IDC_DIEJIA);
        window.focus();
        $("#redgain").attr("value",128);
    }
}

function blurgainBlur(){
    var blurgains = $("#blurgain").val();
    if(isNaN(blurgains) || blurgains < 1 || blurgains > 255) 
    {
        alert(IDC_DIEJIA);
        window.focus();
        $("#blurgain").attr("value",128);
    }
}

function AWBSave(){
    var redgains = $("#redgain").val();
    var blurgains = $("#blurgain").val();
    var AESelect = $("#AESelect").val();
    var AWBSelect = $("#AWBSelect").val();
    var lowlight = lowlightenhance.slider("value");
    var nightfacemode = $("#nightfacemode").val();

    if(isNaN(redgains) || redgains < 0 || redgains > 255)
    {
        alert(IDC_DIEJIA);
        window.focus();
        return;
    }
    if(isNaN(blurgains) || blurgains < 0 || blurgains > 255) 
    {
        alert(IDC_DIEJIA);
        window.focus();
        return;
    }

    var jcpSet = "aeawbblccfg -act set -aeCtrlMode " + AESelect + " -awbCtrlMode " + AWBSelect + " -redGain " + redgains + " -blueGain " + blurgains + " -lowlightenhance " + lowlight + " -nightfacemode " + nightfacemode;
    GetJCP({cmd: jcpSet});
   // GetJCP({cmd: "lenscs -act set -enable "+ ($("#shadowCorrection").is(":checked")==true?1:0)});
    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
}

/*====================================================================
    默认值和保存 functions 
====================================================================*/
var defColorValue = 128;
var defNightluma = 50;
function DefaultColor()
{
    $("#nightluma").slider( "option", "value", defNightluma);
    $("#bright").slider( "option", "value", defColorValue);
    $("#contrast").slider( "option", "value", defColorValue);
    $("#saturation").slider( "option", "value", defColorValue);
    $("#sharpness").slider( "option", "value", defColorValue);
    $("#gain").slider( "option", "value", defColorValue);
    //$("#contrastAgain").slider( "option", "value", 3);
    $("#highLightSuppress").slider( "option", "value", 50);
   
    $("input[name='lampchk'][value=1]").attr("checked", true);
    var jcpstr = "vicfg -act set -nightluma " + defNightluma + " -bright " + defColorValue + " -contrast " + defColorValue + 
                                " -saturation " + defColorValue + " -sharpness " + defColorValue +" -gain " + defColorValue + " -suppress 50 ";
    jcpstr += " -lampfrequency 1 -brightlevel 3";

    GetJCP({cmd: jcpstr});
    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
}

function lamchkSave()
{
    var l = $('input:radio[name="lampchk"]:checked').val();
    GetJCP({cmd: "vicfg -act set -lampfrequency " + l});
    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
}

function showStream(){
    try
    {
        //码流选择
        GetJCP({cmd: "bootargs -act list", ParseJCP: ParseStreamHeightCfg});
        GetJCPList({cmd: "devveopt -act list", ParseJCP: ParseDeviceOptCfg});
        GetJCPList({cmd: "devvecfg -act list", ParseJCP: ParseStreamCfg});
    }
    catch(E){}
}

var masterlen;
var slavelen;

function ParseDeviceOptCfg(jcpstr) {
    try {
        var streamJson = JSON.parse(jcpstr.substring(0, jcpstr.length - 1));
        x264Rate = parseFloat(streamJson["x264"]);
        masterCfgArr = streamJson["master"] || [];
        slaveCfgArr = streamJson["slave"] || [];
        masterlen = masterCfgArr.length;
        slavelen = slaveCfgArr.length;
        mater_html = '';
        slave_html = '';
        for (var i = 0; i < masterCfgArr.length; ++i) {
            mater_html += '<option value="' + masterCfgArr[i]["id"] + '">' + masterCfgArr[i]["name"] + '</option>';
        }
        for (var i = 0; i < slaveCfgArr.length; ++i) {
            slave_html += '<option value="' + slaveCfgArr[i]["id"] + '">' + slaveCfgArr[i]["name"] + '</option>';
        }
        $("#selStreamMaster").html(mater_html);
        $("#selStreamSlave").html(slave_html);
    }
    catch (E){}
}

function ParseStreamHeightCfg(jcpobj){
    if(jcpobj != 'Error'){
        if (jcpobj.maxheight == '2160') {
            $("#selStreamMaster option[value='13']").remove(); //5M
        } else if (jcpobj.maxheight == '1512') { //4M_1
            $("#selStreamMaster option[value='13']").remove(); //5M
            $("#selStreamMaster option[value='15']").remove(); //8M
        } else if (jcpobj.maxheight == '1440') {
            $("#selStreamMaster option[value='16']").remove(); //4M_1
            $("#selStreamMaster option[value='13']").remove(); //5M
            $("#selStreamMaster option[value='15']").remove(); //8M
        } else if (jcpobj.maxheight == '1080') {
              $("#selStreamMaster option[value='16']").remove(); //4M_1
            $("#selStreamMaster option[value='9']").remove(); //3M
              $("#selStreamMaster option[value='12']").remove(); //4M
            $("#selStreamMaster option[value='13']").remove(); //5M
            $("#selStreamMaster option[value='15']").remove(); //8M
        } else if (jcpobj.maxheight == '960') {
            $("#selStreamMaster option[value='16']").remove(); //4M_1
            $("#selStreamMaster option[value='5']").remove(); //1080P
            $("#selStreamMaster option[value='9']").remove(); //3M
            $("#selStreamMaster option[value='12']").remove(); //4M
            $("#selStreamMaster option[value='13']").remove(); //5M
            $("#selStreamMaster option[value='15']").remove(); //8M
        } else if (jcpobj.maxheight == '720') {
            $("#selStreamMaster option[value='16']").remove(); //4M_1
            $("#selStreamMaster option[value='8']").remove(); //960p
            $("#selStreamMaster option[value='5']").remove(); //1080P
            $("#selStreamMaster option[value='9']").remove(); //3M
            $("#selStreamMaster option[value='12']").remove(); //4M
            $("#selStreamMaster option[value='13']").remove(); //5M
            $("#selStreamMaster option[value='15']").remove(); //8M
        } else if (jcpobj.maxheight == '1296' || jcpobj.maxheight == '1536') {
            if (jcpobj.cpu == 'SSC327E') {
                $("#selStreamMaster option[value='15']").remove(); //8M
            } else {
                $("#selStreamMaster option[value='12']").remove(); //4M
                $("#selStreamMaster option[value='13']").remove(); //5M
                $("#selStreamMaster option[value='15']").remove(); //8M
            }
        } else if (jcpobj.maxheight == '1920') {
            $("#selStreamMaster option[value='15']").remove(); //8M
        }

        if (jcpobj.sensor == 'OS04B10' || jcpobj.sensor == 'JXQ03' || jcpobj.sensor == 'SC3335') {
            $("#selStreamSlave option[value='2']").remove();    // no D1
        }
        Set_cookie("maxheight",jcpobj.maxheight);
        Set_cookie("sensor",jcpobj.sensor);
        Set_cookie("cpu",jcpobj.cpu);
    }
}

function showVideo(){
    if(g_is_msie){
      $("#objects").html('<object id="IPCamera" name="IPCamera" CLASSID="CLSID:2319F6E6-ABD3-4b68-BADF-05D8796FA072" width="'+wndWidth+'" height="'+wndHeight+'">></object>');
    }else{
      $("#objects").html('<object id="IPCamera" name="IPCamera" type="application/npipcam" width="'+wndWidth+'" height="'+wndHeight+'">></object>');
    }
    document.IPCamera.IPCSetWindowMode(1);
    var type = GetCookieByKey("ljtypes");
    type = type== -1?1:type;
    document.IPCamera.IPCStartPreviewEx(0, GetCookieByKey("url"), 0, parseInt(type), GetCookieByKey("rtspport"),GetCookieByKey("user"), GetCookieByKey("passwd"), GetCookieByKey("stream"),"V2.00");
}

var position = [[0, 0], [0, 0], [0, 0], [0,0]];    //name, time, bps
var chns;
function SaveOsdStr(index)
{  
    
    if (index < 0 || index > maxOsdName)
    {
        return;
    }
    var osd = $("#osd_" + index).val();

    var myRegExp = /["'<>%;)(&+“”\[\]!！？?\\#￥$^；【】。]/;
    if(myRegExp.test(osd)){
          alert(IDC_OSD_NAME+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    if(12 < osd.length)
    {
        alert(IDC_OSD_TILTE);
        window.focus();
        return;
    }

    var enable = $("#check_"+index).is(":checked") ? 1 : 0;

    var jcpstr = "osdstrcfg -act set -index " + index + " -enable " + enable + " -content \"" + osd + "\"";
    
    GetJCPList({cmd: jcpstr,ParseJCP:function(result){
        if(result != "Error")
        {
            alert(IDC_MSGBOX_SAVEOK);
            window.focus();
        }
        else{
            alert(IDC_MSGBOX_SAVEFAIL);
            window.focus();
        }
    },async:false});
}


//初始化视频参数信息
function initVideoParams()
{   
    try
    {
        GetJCP({cmd: "osdcfg -act list", ParseJCP: ParseOSDCfg});//基础字幕
        GetJCP({cmd: "osdstylecfg -act list", ParseJCP: ParseOSDStyleCfg});//颜色
        GetJCPList({cmd: "osdstrcfg -act list", ParseJCP: ParseOsdStrCfg});//扩展字幕
    }
    catch(E){}
}

function showBaseTitle(){
    initVideoParams();
    window.focus();
}

function showExtendTitle(){
    cleanVideoParam();
    GetJCPList({cmd: "osdstrcfg -act list", ParseJCP: ParseOsdStrCfg_extend});//扩展字幕
    window.focus();
}
 
function showVideoControl(){
    cleanVideoParam();
    GetJCPList({cmd: "vicfg -act list", ParseJCP: ParseViControlCfg});//视频控制
    window.focus();
}

function ParseOSDStyleCfg(jcpstr){
    try{
        $("#colorOption").val(parseInt(jcpstr.colormode));//宽高暂时为实现
    }
    catch(e){}
}


function ParseOSDCfg(jcpstr)
{
    try
    {
        // 名称
        $("#name").val(jcpstr.name);
        OldName = jcpstr.name;
        $("input[name='namechk'][value=" + parseInt(jcpstr.nameen) + "]").attr("checked",true);
        position[0][0] = jcpstr.nameleft;
        position[0][1] = jcpstr.nametop;;
        
        // 时间
        $("input[name='timechk'][value=" + parseInt(jcpstr.timeen) + "]").attr("checked",true);
        position[1][0] = jcpstr.timeleft;
        position[1][1] = jcpstr.timetop;

        // 码率
        $("input[name='bpschk'][value=" + parseInt(jcpstr.bpsen) + "]").attr("checked",true);
        position[2][0] = jcpstr.bpsleft;
        position[2][1] = jcpstr.bpstop;

        document.IPCamera.IPCSetMDModeEx(0, true);
        if(parseInt(jcpstr.nameen)==1){
            document.IPCamera.IPCSetMDAreaRectEx(0,
                                                    position[0][0] * wndWidth / veWidth,
                                                    position[0][1] * wndHeight / veHeight,
                                                    position[0][0] * wndWidth / veWidth,
                                                    position[0][1] * wndHeight / veHeight);
            document.IPCamera.IPCShowMDAreaEx(0, true);
            document.IPCamera.IPCSetMDAreaTitleEx(0, IDC_OSD_OVERLAY_NAME);
        }
        else
        {
            document.IPCamera.IPCShowMDAreaEx(0, false);
        }

        if(parseInt(jcpstr.timeen)==1){
            document.IPCamera.IPCSetMDAreaRectEx(1,
                                                    position[1][0] * wndWidth / veWidth,
                                                    position[1][1] * wndHeight / veHeight,
                                                    position[1][0] * wndWidth / veWidth,
                                                    position[1][1] * wndHeight / veHeight);
            document.IPCamera.IPCShowMDAreaEx(1, true);
            document.IPCamera.IPCSetMDAreaTitleEx(1, IDC_OSD_OVERLAY_TIME);
        }
        else
        {
            document.IPCamera.IPCShowMDAreaEx(1, false);
        }

        if(parseInt(jcpstr.bpsen)==1){
            document.IPCamera.IPCSetMDAreaRectEx(2,
                                                    position[2][0] * wndWidth / veWidth,
                                                    position[2][1] * wndHeight / veHeight,
                                                    position[2][0] * wndWidth / veWidth,
                                                    position[2][1] * wndHeight / veHeight);
            document.IPCamera.IPCShowMDAreaEx(2, true);
            document.IPCamera.IPCSetMDAreaTitleEx(2, "bps");
        }
        else
        {
            document.IPCamera.IPCShowMDAreaEx(2, false);
        }
    }
    catch(e){return e;}
}

var OsdName = 1;//最大叠加的OSD个数
function ParseOsdStrCfg(jcpstr)
{
    try
    {
        var extArr = jcpstr.split("#");
        for(var i=0; i<OsdName;i++){
            var extObj = parse_jcp_content(extArr[i]);
            /*$("#check_" + i).prop("checked", (parseInt(extObj.enable) == 0 ? false : true));
            $("#xpos_" + i).val(extObj.left);
            $("#ypos_" + i).val(extObj.top);
            $("#osd_" + i).val(extObj.content);*/
            $("input[name='osdchk'][value=" + parseInt(extObj.enable) + "]").attr("checked",true);
            $("#osdname").val(extObj.content); 
            if(i == 0){
                $("#osd_font").val(extObj.size);
            }
            position[3][0] =   extObj.left;  
            position[3][1] =   extObj.top;      

            if(parseInt(extObj.enable)==1){
                document.IPCamera.IPCSetMDAreaRectEx(3,
                                                    position[3][0] * wndWidth / veWidth,
                                                        position[3][1] * wndHeight / veHeight,
                                                        position[3][0] * wndWidth / veWidth,
                                                        position[3][1] * wndHeight / veHeight);
                document.IPCamera.IPCShowMDAreaEx(3, true);
                document.IPCamera.IPCSetMDAreaTitleEx(3, IDC_OSD_OVERLAY_OSD);
            }
            else
            {
                document.IPCamera.IPCShowMDAreaEx(3, false);
            } 
        }
    }catch(E){}
}


var maxOsdName = 8;//最大叠加的OSD个数
function ParseOsdStrCfg_extend(jcpstr)
{
    try
    {
        var extArr = jcpstr.split("#");
        for(var i=0; i<maxOsdName;i++){
            var extObj = parse_jcp_content(extArr[i]);
            $("#check_" + i).prop("checked", (parseInt(extObj.enable) == 0 ? false : true));
            $("#xpos_" + i).val(extObj.left);
            $("#ypos_" + i).val(extObj.top);
            $("#osd_" + i).val(extObj.content);
        }
    }catch(E){}
}


function ParseStreamCfg(jcpstr){
    try
    {
        var streamArr = jcpstr.split("#");
        
        //主码流
        var masterObj = parse_jcp_content(streamArr[0]);
        $("input[name='rdMaster'][value=" + masterObj.enable + "]").attr("checked",true);//启用
        $("#selStreamMaster").val(masterObj.vencsize); //分辨率
        $("#frmrateMaster").val(masterObj.fps); //帧率
        $("#bitrateMaster").val(masterObj.bps); //码率
        $("#frmintrMaster").val(masterObj.gop); //帧间隔
        $("#selRateCtrlMaster").val(masterObj.fixbps); //码率控制
        //$("#selFirstMaster").val(masterObj.fixfps); //编码模式
        $("#selVeEncMaster").val(masterObj.codec); //压缩模式

        //从码流
        var slaveObj = parse_jcp_content(streamArr[1]);
        $("input[name='rdSlave'][value=" + slaveObj.enable + "]").attr("checked",true);//启用
        $("#selStreamSlave").val(slaveObj.vencsize); //分辨率
        $("#frmrateSlave").val(slaveObj.fps); //帧率
        $("#bitrateSlave").val(slaveObj.bps); //码率
        $("#frmintrSlave").val(slaveObj.gop); //帧间隔
        $("#selRateCtrlSlave").val(slaveObj.fixbps); //码率控制
        //$("#selFirstSlave").val(slaveObj.fixfps); //编码模式
        $("#selVeEncSlave").val(slaveObj.codec); //压缩模式

        if(parseInt(masterObj.enable)==0){
            DisableMaster(1);
        }

        if(parseInt(slaveObj.enable)==0){
            DisableSlave(1);
        }

        ChgStream(0,3); // 3 的值是为了防止重置默认值
        ChgStream(1,3);
    }catch(E){}
}

/*====================================================================
    设置基础字幕位置
====================================================================*/
function SetOSDPosition(type)
{
    if(type==0)
    {
        CommonSetOSDPosition("namechk", 0, IDC_OSD_OVERLAY_NAME);
    }
    else if (type==1)
    {
        CommonSetOSDPosition("timechk", 1, IDC_OSD_OVERLAY_TIME);
    }
    else if (type==2)
    {
        CommonSetOSDPosition("bpschk", 2, "bps");
    }
    else if (type==3)
    {
        CommonSetOSDPosition("osdchk", 3, IDC_OSD_OVERLAY_OSD);
    }
    else if(type==-1){
        CommonSetOSDPosition("namechk", 0, IDC_OSD_OVERLAY_NAME);
        CommonSetOSDPosition("timechk", 1, IDC_OSD_OVERLAY_TIME);
        CommonSetOSDPosition("bpschk", 2, "bps");
        CommonSetOSDPosition("osdchk", 3, IDC_OSD_OVERLAY_OSD);
    }
}

// SetOSDPosition 方法中提取公用方法
function CommonSetOSDPosition(inputName, positionItem, AreaTitleEx){
    if (parseInt($('input:radio[name="' + inputName + '"]:checked').val())==1)
    {
        document.IPCamera.IPCSetMDModeEx(positionItem, true);
        document.IPCamera.IPCSetMDAreaRectEx(positionItem,
                                                position[positionItem][0] * wndWidth / veWidth,
                                                position[positionItem][1] * wndHeight / veHeight,
                                                position[positionItem][0] * wndWidth / veWidth,
                                                position[positionItem][1] * wndHeight / veHeight);
        document.IPCamera.IPCShowMDAreaEx(positionItem, true);
        document.IPCamera.IPCSetMDAreaTitleEx(positionItem, AreaTitleEx);
    }
    else
    {
        document.IPCamera.IPCShowMDAreaEx(positionItem, false);
    }
}

/*====================================================================
    保存页面参数 functions 
====================================================================*/
function UpdatePosition()
{
    var ret = 0;
    var nameen = parseInt($('input:radio[name="namechk"]:checked').val());
    var timeen = parseInt($('input:radio[name="timechk"]:checked').val());
    var bpsen = parseInt($('input:radio[name="bpschk"]:checked').val());
    var osdenable = parseInt($('input:radio[name="osdchk"]:checked').val());

    if(nameen == 1 && !CommonUpdatePosition("namechk", 0, 0))
    {
        return false;
    }

    if(timeen == 1 && !CommonUpdatePosition("timechk", 1, 1))
    {
        return false;
    }

    if(bpsen == 1 && !CommonUpdatePosition("bpschk", 2, 2))
    {
        return false;
    }

    if(osdenable == 1 && !CommonUpdatePosition("osdchk", 3, 3))
    {
        return false;
    }

    return true;
}

function CommonUpdatePosition(inputName, positionItem, wndRect)
{
    if($('input:radio[name="'+inputName+'"]:checked').val())
    {
        winPosStr = document.IPCamera.IPCGetVideoWndRect(wndRect);
        ret = GetRtspKeyStr(winPosStr, "left");
        if (ret == -2)
        {
            return false;
        }
        position[positionItem][0] = parseInt(parseInt(ret) * veWidth / wndWidth);
        position[positionItem][0] = position[positionItem][0] > 1920 ? 1920 : position[positionItem][0];
        position[positionItem][0] = position[positionItem][0] < 0    ? 0    : position[positionItem][0];

        ret = GetRtspKeyStr(winPosStr, "top");
        if (ret == -2)
        {
            return false;
        }
        if(parseInt(ret) > parseInt(wndHeight / 2))
        {
            ret = parseInt(ret) + 30;
        }
        position[positionItem][1] = parseInt(parseInt(ret) * veHeight / wndHeight);
        position[positionItem][1] = position[positionItem][1] > 1080 ? 1080 : position[positionItem][1];
        position[positionItem][1] = position[positionItem][1] < 0    ? 0    : position[positionItem][1];
        return true;
    }
}


function SaveBasic()
{
    var myRegExp = new RegExp("[\\u4e00-\\u9fa5]+\\s+[\\u4e00-\\u9fa5]+|[\\w]+\\b\\s+[\\w]+");  //匹配name中是否含有空格并替换为''
    var nameen = parseInt($('input:radio[name="namechk"]:checked').val());
    var timeen = parseInt($('input:radio[name="timechk"]:checked').val());
    var bpsen = parseInt($('input:radio[name="bpschk"]:checked').val());
    var color = parseInt($("#colorOption").find("option:selected").val());

    var name = $("#name").val();

    var myRegExp = /["'<>%;)(&+“”\[\]!！？?\\#￥$^；【】。]/;
    if(myRegExp.test(name)){
          alert(IDC_NAME+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    if (0 >= name.length || 12 < name.length)
    {
        alert(IDC_NAME_MSG + IDC_NAME_CONTENT);
        window.focus();
        return false;
    }
    
    var osd = $("#osdname").val();
    if(myRegExp.test(osd)){
          alert(IDC_OSD_NAME+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    if(12 < osd.length)
    {
        alert(IDC_NAME_DIS_DIEJIA + IDC_NAME_CONTENT);
        window.focus();
        return;
    }
    var osd_font = $("#osd_font").val();
    var osdenable = parseInt($('input:radio[name="osdchk"]:checked').val());

    //获取OSD位置信息
     UpdatePosition();
     var jcpstr = "osdcfg -act set ";
     jcpstr += " -nameen " + nameen + " -nameleft " + position[0][0] + " -nametop " + position[0][1] + " -name \"" + name+"\"";
   
     GetJCPList({cmd: jcpstr});

     var jcpstr2 = "osdcfg -act set ";
     jcpstr2 += " -timeen " + timeen + " -timeleft " + position[1][0] + " -timetop " + position[1][1];
     GetJCPList({cmd: jcpstr2});

     var jcpstr3 = "osdcfg -act set ";
     jcpstr3 += " -bpsen " + bpsen + " -bpsleft " + position[2][0] + " -bpstop " + position[2][1];
     GetJCPList({cmd: jcpstr3});

     var jcpstr4 = "osdstylecfg -act set ";
     jcpstr4 += " -colormode " + color;
     GetJCPList({cmd: jcpstr4});

     var jcpstr5 = "osdstrcfg -act set -size " + osd_font + " -index 0 -enable " + osdenable + " -content \"" + osd + "\" -left " + position[3][0] + " -top " + position[3][1];
     //var jcpstr5 = "osdstrcfg -act set -size " + osd_font;
     GetJCPList({cmd: jcpstr5});

     alert(IDC_MSGBOX_SAVEOK);
     window.focus();
     return 0;
}


// 开启关闭
function DisableMaster(show)
{
    if (show == 1)
    {
        $("#frmrateMaster").attr("disabled", true);
        $("#bitrateMaster").attr("disabled", true);
        $("#frmintrMaster").attr("disabled", true);
        $("#selStreamMaster").attr("disabled", true);
        $("#selRateCtrlMaster").attr("disabled", true);
        //$("#selFirstMaster").attr("disabled", true);
        //$("#selVeEncMaster").attr("disabled", true);
    }
    else
    {
        $("#frmrateMaster")[0].disabled = "";
        $("#bitrateMaster")[0].disabled = "";
        $("#frmintrMaster")[0].disabled = "";
        $("#selStreamMaster")[0].disabled = "";
        $("#selRateCtrlMaster")[0].disabled = "";
        //$("#selFirstMaster")[0].disabled = "";
        //$("#selVeEncMaster")[0].disabled = "";
    }
}

function DisableSlave(show)
{
    if (show == 1)
    {
        $("#frmrateSlave").attr("disabled", true);
        $("#bitrateSlave").attr("disabled", true);
        $("#frmintrSlave").attr("disabled", true);
        $("#selStreamSlave").attr("disabled", true);
        $("#selRateCtrlSlave").attr("disabled", true);
        //$("#selFirstSlave").attr("disabled", true);
        //$("#selVeEncSlave").attr("disabled", true);
    }
    else
    {
         $("#frmrateSlave")[0].disabled = "";
         $("#bitrateSlave")[0].disabled = "";
         $("#frmintrSlave")[0].disabled = "";
         $("#selStreamSlave")[0].disabled = "";
         $("#selRateCtrlSlave")[0].disabled = "";
         //$("#selFirstSlave")[0].disabled = "";
         //$("#selVeEncSlave")[0].disabled = "";
    }

}
    
//开关主从码流时
function ChgOpen(type){
    var rdmaster = parseInt($('input:radio[name="rdMaster"]:checked').val());
    var rdslave = parseInt($('input:radio[name="rdSlave"]:checked').val());

    //两路码流不能相同，主码流尺寸要大于从码率尺寸,必须有一路打开。
    if (rdmaster == 0 && rdslave == 0)
    {
        alert(IDC_CHANNEL_NUM_MIN);
        window.focus();
        if(type==0){
            DisableMaster(0);
            $("input[name='rdMaster'][value='1']").attr("checked",true);
        }else if(type==1){
            DisableSlave(0);
            $("input[name='rdSlave'][value='1']").attr("checked",true);
        }
        return;
    }
}

function setSlaveObj(vesize) {
    $("#selStreamSlave").val(vesize);
    var m = $("#selStreamSlave").val();
    for (var i = 0; i < masterlen; ++i)
    {
        if (m == masterCfgArr[i]["id"])
        {
            $("#bitrateSlave").val(masterObj[i]["bps_def"]);
            $("#frmrateSlave").val(masterObj[i]["fps_def"]);
            $("#frmintrSlave").val(masterObj[i]["gop_def"]);
        }
    }
}

function getSelectMasterObj() {
    var m = $("#selStreamMaster").val();
    for (var i = 0; i < masterlen; ++i)
    {
        if (m == masterCfgArr[i]["id"])
        {
            return masterCfgArr[i];
        }
    }
}

function getSelectSlaveObj() {
    var s = $("#selStreamSlave").val();
    for (var i = 0; i < slavelen; ++i)
    {
        if (s == slaveCfgArr[i]["id"])
        {
            return slaveCfgArr[i];
        }
    }
}

//主码流码率、帧率、I帧间隔取值范围
function changeMasterStreamTip() {
    var masterObj = getSelectMasterObj();
    $("#spanbitrateMaster").html("(" + masterObj["bps_min"] + "~" + masterObj["bps_max"] + ")");
    $("#frmrateMasterTip").html("(" + masterObj["fps_min"] + "~" + masterObj["fps_max"] + ")");
    $("#spanfrmintrMaster").html("(" + masterObj["gop_min"] + "~" + masterObj["gop_max"] + ")");
}

//子码流码率、帧率、I帧间隔取值范围
function changeSlaveStreamTip() {
    var slaveObj = getSelectSlaveObj();
    $("#spanbitrateSlave").html("(" + slaveObj["bps_min"] + "~" + slaveObj["bps_max"] + ")");
    $("#frmrateSlaveTip").html("(" + slaveObj["fps_min"] + "~" + slaveObj["fps_max"] + ")");
    $("#spanfrmintrSlave").html("(" + slaveObj["gop_min"] + "~" + slaveObj["gop_max"] + ")");
}

//主码流码率、帧率、I帧间隔默认值
function changeMasterStreamDefValue() {
    var masterObj = getSelectMasterObj();
    $("#bitrateMaster").val(masterObj["bps_def"]);
    $("#frmrateMaster").val(masterObj["fps_def"]);
    $("#frmintrMaster").val(masterObj["gop_def"]);
}

//子码流码率、帧率、I帧间隔默认值
function changeSlaveStreamDefValue() {
    var slaveObj = getSelectSlaveObj();
    $("#bitrateSlave").val(slaveObj["bps_def"]);
    $("#frmrateSlave").val(slaveObj["fps_def"]);
    $("#frmintrSlave").val(slaveObj["gop_def"]);
}

var rmed_d1 = false;  // 移除过置为 true 移除过才能添加
var rmed_vga = false;

//改变主从码流分辨率时
function ChgStream(chn,flag)
{
    var s = $("#selStreamMaster").val();
                
    if(chn==0){
        if (flag != 3) { //需要判断，否则会重复显示默认值
            changeMasterStreamDefValue();
        }
        changeMasterStreamTip();
        
        //当主码流选择D1，从码流不能出现D1
        if(2 == parseInt(s)){
            if(2 == $("#selStreamSlave").val()){
                setSlaveObj(7);
            }
            
            //删从码流的D1
            $("#selStreamSlave > option").each(function (){
                if (parseInt($(this).attr("value")) == 2){
                    $(this).remove();
                    rmed_d1 = true;
                }
            });

            //如果vga删除了，则恢复
            var vExist = 0;
            $("#selStreamSlave > option").each(function (){
                if (parseInt($(this).attr("value")) == 7){
                    vExist ++;
                }
            });

            if (vExist == 0){
                $("#selStreamSlave").append('<option value="7">' + 'VGA' + '</option>');
            }
        }

        //当主码流选择VGA，从码流不能出现VGA
        if(7 == parseInt(s)){
            if($("#selStreamSlave").val()==2 || $("#selStreamSlave").val()==7){
                setSlaveObj(1);
            }

            //删从码流的D1 VGA
            $("#selStreamSlave > option").each(function (){
                if (parseInt($(this).attr("value")) == 2 || parseInt($(this).attr("value")) == 7){
                    $(this).remove();
                    rmed_vga = true;
                }
            });
        }else{
            //如果vga,d1删除了，则恢复
            var d1Exist=0,vExist=0;
            $("#selStreamSlave > option").each(function (){
                if(parseInt($(this).attr("value")) == 2){
                    d1Exist ++;
                }
                if(parseInt($(this).attr("value")) == 7){
                    vExist ++;
                }
            });

            if (d1Exist == 0 && rmed_d1){
                if(2 != parseInt(s)) {
                    var sensor = GetCookieByKey("sensor");
                    if (sensor != 'OS04B10' && sensor != 'JXQ03' && sensor != 'SC3335') {
                        $("#selStreamSlave").append('<option value="2">' + 'D1' + '</option>');
                    }
                }
            }

            if (vExist == 0 && rmed_vga)
            {
                $("#selStreamSlave").append('<option value="7">' + 'VGA' + '</option>');
            }
        }
        
    } else if(chn == 1) {
        var s1 = $("#selStreamSlave").val();
        if(flag != 3){
            changeSlaveStreamDefValue();
        }
        changeSlaveStreamTip();
    }
}
//保存码流设置
function SaveStream(){
    var rdmaster = parseInt($('input:radio[name="rdMaster"]:checked').val());
    var rdslave = parseInt($('input:radio[name="rdSlave"]:checked').val());

    if (rdmaster == 0 && rdslave == 0) {
        alert(IDC_CHANNEL_NUM_MIN);
        window.focus();
        return;
    }

    //分辨率和码率验证
    var masterObj = getSelectMasterObj();
    var slaveObj = getSelectSlaveObj();

    var ms = $("#selStreamMaster").val();
    var ss = $("#selStreamSlave").val();
    var mb = $("#bitrateMaster").val(); //bps
    var sb = $("#bitrateSlave").val();
    var mRate = $("#frmrateMaster").val(); //fps
    var sRate = $("#frmrateSlave").val();
    var mIntr = $("#frmintrMaster").val(); //gop
    var sIntr = $("#frmintrSlave").val();
    
    // 主码流参数判断
    if (mRate > masterObj["fps_max"] || mRate < masterObj["fps_min"]) { // fps
        alert(IDC_STREAM_MASTER + " " + IDC_FPS_SCOPE + masterObj["fps_min"] + "~" + masterObj["fps_max"]);
        window.focus();
        return;
    }

    if (mIntr > masterObj["gop_max"] || mIntr < masterObj["gop_min"]) { // gop
        alert(IDC_STREAM_MASTER + " " + IDC_INTERVAL.replace(/&nbsp;/g, "") + masterObj["gop_min"] + "~" + masterObj["gop_max"]);
        window.focus();
        return;
    }
    
    if ((mb > masterObj["bps_max"] || mb < masterObj["bps_min"])) { // bps
        alert(IDC_STREAM_MASTER + " " + IDC_BPS.replace(/&nbsp;/g, "") + masterObj["bps_min"] + "~" + masterObj["bps_max"]);
        window.focus();
        return;
    }

    // 子码流参数判断
    if (sRate > slaveObj["fps_max"] || sRate < slaveObj["fps_min"]) {
        alert(IDC_STREAM_SLAVE + " " + IDC_FPS_SCOPE + slaveObj["fps_min"] + "~" + slaveObj["fps_max"]);
        window.focus();
        return;
    }
    
    if (sIntr > slaveObj["gop_max"] || sIntr < slaveObj["gop_min"]) { // gop
        alert(IDC_STREAM_SLAVE + " " + IDC_INTERVAL.replace(/&nbsp;/g, "") + slaveObj["gop_min"] + "~" + slaveObj["gop_max"]);
        window.focus();
        return;
    }

    if ((sb > slaveObj["bps_max"] || sb < slaveObj["bps_min"])) { // bps
        alert(IDC_STREAM_SLAVE + " " + IDC_BPS.replace(/&nbsp;/g, "") + slaveObj["bps_min"] + "~" + slaveObj["bps_max"]);
        window.focus();
        return;
    }

    //主码流
    var jcpstr = "devvecfg -act set -id 0 -enable "+rdmaster;
    jcpstr += " -vencsize "+$("#selStreamMaster").val();
    jcpstr += " -fps "+$("#frmrateMaster").val();
    jcpstr += " -bps "+$("#bitrateMaster").val();
    jcpstr += " -gop "+$("#frmintrMaster").val();
    jcpstr += " -fixbps "+$("#selRateCtrlMaster").val();
    //jcpstr += " -fixfps "+$("#selFirstMaster").val();
    jcpstr += " -codec "+$("#selVeEncMaster").val();

    //从码流
    jcpstr += " -id 1 -enable "+rdslave;
    jcpstr += " -vencsize "+$("#selStreamSlave").val();
    jcpstr += " -fps "+$("#frmrateSlave").val();
    jcpstr += " -bps "+$("#bitrateSlave").val();
    jcpstr += " -gop "+$("#frmintrSlave").val();
    jcpstr += " -fixbps "+$("#selRateCtrlSlave").val();
    //jcpstr += " -fixfps "+$("#selFirstSlave").val();
    jcpstr += " -codec "+$("#selVeEncSlave").val();
    GetJCPList({cmd: jcpstr, timeout: 5000, ParseJCP: function(result){
        if(result != "Error")
        {
            //var stream_size = rdmaster == 1 ? "stream1" : "stream2";
            //主码流修改为不能关闭，所以除主页视频外永远显示主码流
            Set_cookie("stream", "stream1");
            Set_cookie("master_enb",parseInt(rdmaster));
            Set_cookie("master_stream",$("#selStreamMaster").val())
            Set_cookie("slave_enb",parseInt(rdslave));
            Set_cookie("slave_stream",$("#selStreamSlave").val())
            alert(IDC_MSGBOX_SAVEOK);
        }
        else
        {
            alert(IDC_MSGBOX_SAVEFAIL);
        }
        window.focus();
    }});
}

function PreviewReverse(){
    var i = parseInt($('input:radio[name="videoreverse"]:checked').val());
    GetJCPList({cmd: "vicfg -act set -reverse " + i});
}

function ParseViControlCfg(jcpstr)
{
    var ret;
    try
    {
        if (isBlank(jcpstr))
        {
            throw -1;
        }
        
        ret = GetRtspKeyStr(jcpstr, 'reverse');
        if (ret == -2)
        {
            throw -2;
        }
        if (parseInt(ret) < 0 || parseInt(ret) > 4)
        {
            $("input[name='videoreverse'][value=0]").attr("checked",true);
        }
        else
        {
            $("input[name='videoreverse'][value=" + parseInt(ret) + "]").attr("checked",true);
        }
    }
    catch(e){}
}

var strengthSlider;
function show3dNoise(){
     strengthSlider = $("#StrengthSlider").slider({
        slide: function (event, ui)
        {
            $("#tdStrengthValue").html(ui.value);
        },
        min:0,
        max:100
     });
   
    GetJCP({cmd: "denoisecfg -act list", ParseJCP: function(jcpstr){
            $("input[name='denoise'][value=" + parseInt(jcpstr.enable) + "]").attr("checked",true);
            strengthSlider.slider("value", parseInt(jcpstr.mode));
            $("#tdStrengthValue").html(jcpstr.mode);
    }});
}

var encodeSettingArr = [[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0]];
function showEncodeSetting(){
    //删除不匹配的码流尺寸
    var maxheight = GetCookieByKey("maxheight");
    if(maxheight == '1080'){
        $("#selEncodeSize option[value='9']").remove(); //3M
    }else if(maxheight == '960'){
        $("#selEncodeSize option[value='5']").remove(); //1080P
        $("#selEncodeSize option[value='9']").remove(); //3M
    }else if(maxheight == '720'){
        $("#selEncodeSize option[value='8']").remove(); //960p
        $("#selEncodeSize option[value='5']").remove(); //1080P
        $("#selEncodeSize option[value='9']").remove(); //3M
    }else if(maxheight == '1296' || maxheight == '1536'){
        //预留300万，500万像素
    }

    GetJCPList({cmd: "veprofile -act list", ParseJCP: function(jcpstr){
        var extArr = jcpstr.split("#");
        for(var i=0; i<10;i++){
            var extObj = parse_jcp_content(extArr[i]);
            encodeSettingArr[i][0] = parseInt(extObj.vesize);
            encodeSettingArr[i][1] = parseInt(extObj.profile);
            //encodeSettingArr[i][2] = parseInt(extObj.level);
        }
        $("#selEncodeSize").val(encodeSettingArr[1][0]);
        $("#selChildEncodeType").val(encodeSettingArr[1][1]);
        //$("#selEncodeLevel").val(encodeSettingArr[1][2]);
    }});
}

//扩展配置->编码设置->编码尺寸添加选择事件
function changeEncodeSize(v){
    $("#selChildEncodeType").val(encodeSettingArr[parseInt(v)][1]);
    //$("#selEncodeLevel").val(encodeSettingArr[parseInt(v)][2]);
}

//保存3d数字降噪
function SaveExtCfgNoise(){
    var i = parseInt($('input:radio[name="denoise"]:checked').val());
    GetJCPList({cmd: "denoisecfg -act set -enable " + i + " -mode "+ parseInt($("#tdStrengthValue").text()) });

    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
}

//保存编码设置
function SaveExtCfgEncode(){
    var vesize = $("#selEncodeSize").val();
    var profile = $("#selChildEncodeType").val();
    //var level = $("#selEncodeLevel").val();

    encodeSettingArr[parseInt(vesize)][0] = parseInt(vesize);
    encodeSettingArr[parseInt(vesize)][1] = parseInt(profile);
    //encodeSettingArr[parseInt(vesize)][2] = parseInt(level);

    var jcpstr = "veprofile -act set -vesize " + vesize +" -profile "+profile;
    //+" -level "+level ;
    GetJCPList({cmd: jcpstr});

    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
}

function showHikvisonNVR(){
    GetJCP({cmd: "hk32chncfg -act list", ParseJCP: function(jcpstr){
        $("input[name='nvroise'][value=" + parseInt(jcpstr.enable) + "]").attr("checked",true);
    }});
}

function SaveHikvisonNVR(v){
    GetJCPList({cmd: "hk32chncfg -act set -enable " + v });

    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
}

var videoMaskArr = [
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
];

var videoMaskColor;


//视频遮挡区域选择,启用隐藏触发事件
function AreaMaskSelect(){
     var v = parseInt($("#selAreaMask").val());
     var masken = videoMaskArr[v][0];
     if(masken==1){
        $("#checkMask").prop("checked",true);
        document.IPCamera.IPCSetMDModeEx(0, true);
        document.IPCamera.IPCSetMDAreaRectEx(parseInt(0),
                                                videoMaskArr[v][1] * wndWidth / veWidth,
                                                videoMaskArr[v][2]  * wndHeight / veHeight,
                                                videoMaskArr[v][3]  * wndWidth / veWidth,
                                                videoMaskArr[v][4]  * wndHeight / veHeight);

        document.IPCamera.IPCShowMDAreaEx(0, true);
        document.IPCamera.IPCSetMDAreaTitleEx(0, parseInt(v)+1);
    }
    else {     
        $("#checkMask").prop("checked",false);
        document.IPCamera.IPCShowMDAreaEx(0, false);
    }
}

//视频遮挡启动
function MaskEnableSelect(){
     var v = $("#selAreaMask").val();
     var enable = $("#checkMask").is(":checked");
     
     if(enable){
        document.IPCamera.IPCSetMDModeEx(0, true);
        document.IPCamera.IPCSetMDAreaRectEx(parseInt(0),
                                                parseInt(videoMaskArr[v][1] * wndWidth / veWidth),
                                                parseInt(videoMaskArr[v][2]  * wndHeight / veHeight),
                                                parseInt(videoMaskArr[v][3]  * wndWidth / veWidth),
                                                parseInt(videoMaskArr[v][4]  * wndHeight / veHeight));

        document.IPCamera.IPCShowMDAreaEx(0, true);
        document.IPCamera.IPCSetMDAreaTitleEx(0, parseInt(v)+1);
    }
    else {     
        document.IPCamera.IPCShowMDAreaEx(0, false);
    }
}

//初始化视频遮挡信息
function showVideoMask(){
    if(has_dome == 1){
        $("#ShieldingColor").attr("disabled",true);
       
        for(var i = 0 ; i < shelter; i++){
            document.getElementById("ShieldingIndex").options.add(new Option(i + 1, "index" + i))
        }
    }else{
        GetJCPList({cmd: "videomaskcfg -act list", ParseJCP: function(jcpstr){
            var extArr = jcpstr.split("#");
            for(var i=0; i<8;i++){
                var extObj = parse_jcp_content(extArr[i]);
                videoMaskArr[i][0] = extObj.masken; 
                videoMaskArr[i][1] = extObj.left;
                videoMaskArr[i][2] = extObj.top;
                videoMaskArr[i][3] = extObj.right;
                videoMaskArr[i][4] = extObj.bottom;

                if(i==0){
                     $("#selMaskColor").val(extObj.color);
                     $("#selMaskColor").css("background", $("#selMaskColor option:eq(" + extObj.color + ")").attr("bgvalue"));
                }
            }
            AreaMaskSelect();
        }});
    }
}

//视频遮挡颜色变化
function SetColorChg(){
    $("#selMaskColor").css("background", $("#selMaskColor option:eq(" + $("#selMaskColor").val() + ")").attr("bgvalue"));
    window.focus();
}

//保存视频遮挡
function SaveVideoMask(){
    var v =  parseInt($("#selAreaMask").val());
    var masken = $("#checkMask").is(":checked")?1:0;
    var c = $("#selMaskColor").val();
    var winPosStr = document.IPCamera.IPCGetVideoWndRect(0);

    var l = GetRtspKeyStr(winPosStr, "left");
    var t = GetRtspKeyStr(winPosStr, "top");
    var r = GetRtspKeyStr(winPosStr, "right");
    var b = GetRtspKeyStr(winPosStr, "bottom");

    l = parseInt(l*veWidth/wndWidth);
    t = parseInt(t*veHeight/wndHeight);
    r = parseInt(r*veWidth/wndWidth);
    b = parseInt(b*veHeight/wndHeight);

    videoMaskArr[v][0] = masken;
    videoMaskArr[v][1] = l;
    videoMaskArr[v][2] = t;
    videoMaskArr[v][3] = r;
    videoMaskArr[v][4] = b;

    var jcpstr = "videomaskcfg -act set -maskid "+v+" -masken "+masken+" -color "+c+"  -left "+l+" -top "+t+" -right "+r+" -bottom "+b;

    GetJCP({cmd: jcpstr,ParseJCP: function(jcpstr){
        if(jcpstr=='Error'){
           alert(IDC_MSGBOX_SAVEFAIL);
        }else{
           alert(IDC_MSGBOX_SAVEOK);
        }
    }});
    window.focus();
}

//删除
function DelVideoMask(){
    var v =  parseInt($("#selAreaMask").val());
    document.IPCamera.IPCSetMDModeEx(0, true);
    document.IPCamera.IPCShowMDAreaEx(0, false);
    videoMaskArr[v][0] = 0;
    videoMaskArr[v][1] = 0;
    videoMaskArr[v][2] = 0;
    videoMaskArr[v][3] = 0;
    videoMaskArr[v][4] = 0;
    var jcpstr = "videomaskcfg -act set -maskid "+v+" -masken 0  -left 0 -top 0 -right 0 -bottom 0";
    GetJCP({cmd: jcpstr,ParseJCP: function(jcpstr){
        if(jcpstr=="Error"){
            alert(IDC_MSGBOX_DELETEFAIL);
            window.focus();
        }else{
            $("#checkMask").prop("checked",false);
            alert(IDC_MSGBOX_DELETEOK);
            window.focus();
        }
    }});
}

//-----------------------------------------ROI设置
var roiMaskArr = [
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
    [0,0,0,0,0],
];

//ROI启动
function RoiEnableSelect(){
     var v = $("#selRoi").val();
     var enable = $("#checkRoi").is(":checked");
     
     if(enable){
        document.IPCamera.IPCSetMDModeEx(0, true);
        document.IPCamera.IPCSetMDAreaRectEx(parseInt(0),
                                                parseInt(roiMaskArr[v][1] * wndWidth / veWidth),
                                                parseInt(roiMaskArr[v][2]  * wndHeight / veHeight),
                                                parseInt(roiMaskArr[v][3]  * wndWidth / veWidth),
                                                parseInt(roiMaskArr[v][4]  * wndHeight / veHeight));

        document.IPCamera.IPCShowMDAreaEx(0, true);
        document.IPCamera.IPCSetMDAreaTitleEx(0, parseInt(v)+1);
    }
    else {     
        document.IPCamera.IPCShowMDAreaEx(0, false);
    }
}

//初始化视频遮挡信息
function showRoiInfo(){
    GetJCPList({cmd: "roicfg -act list", ParseJCP: function(jcpstr){
        var extArr = jcpstr.split("#");
        for(var i=0; i<8;i++){
            var extObj = parse_jcp_content(extArr[i]);
            roiMaskArr[i][0] = parseInt(extObj.enable); 
            roiMaskArr[i][1] = parseInt(extObj.left);
            roiMaskArr[i][2] = parseInt(extObj.top);
            roiMaskArr[i][3] = parseInt(extObj.right);
            roiMaskArr[i][4] = parseInt(extObj.bottom);
        }
        RoiSelect();

    }});
}

var roi_index = 0;
function changeRoiIndex(){
    roi_index = parseInt($("#roi_option").val());
}

//ROI选择区域
function RoiSelect(){
    var v = parseInt($("#selRoi").val());
   
    var masken = roiMaskArr[v][0];
    if(masken==1){
        $("#checkRoi").prop("checked",true);
        document.IPCamera.IPCSetMDModeEx(0, true);
        document.IPCamera.IPCSetMDAreaRectEx(parseInt(0),
                                                roiMaskArr[v][1] * wndWidth / veWidth,
                                                roiMaskArr[v][2]  * wndHeight / veHeight,
                                                roiMaskArr[v][3]  * wndWidth / veWidth,
                                                roiMaskArr[v][4]  * wndHeight / veHeight);

        document.IPCamera.IPCShowMDAreaEx(0, true);
        document.IPCamera.IPCSetMDAreaTitleEx(0, parseInt(v)+1);
    }
    else {     
        $("#checkRoi").prop("checked",false);
        document.IPCamera.IPCShowMDAreaEx(0, false);
    }
}

//保存视频遮挡
function SaveRoi(){
    var v =  parseInt($("#selRoi").val());
    var masken = $("#checkRoi").is(":checked")?1:0;

    var winPosStr = document.IPCamera.IPCGetVideoWndRect(0);

    var l = GetRtspKeyStr(winPosStr, "left");
    var t = GetRtspKeyStr(winPosStr, "top");
    var r = GetRtspKeyStr(winPosStr, "right");
    var b = GetRtspKeyStr(winPosStr, "bottom");

    l = parseInt(l*veWidth/wndWidth);
    t = parseInt(t*veHeight/wndHeight);
    r = parseInt(r*veWidth/wndWidth);
    b = parseInt(b*veHeight/wndHeight);

    roiMaskArr[v][0] = masken;
    roiMaskArr[v][1] = l;
    roiMaskArr[v][2] = t;
    roiMaskArr[v][3] = r;
    roiMaskArr[v][4] = b;

    var jcpstr = "roicfg -act set -id "+v+" -enable "+masken+" -left "+l+" -top "+t+" -right "+r+" -bottom "+b;

    GetJCPList({cmd: jcpstr});

    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
}

//删除
function DelRoi(){
    var v =  parseInt($("#selRoi").val());
    document.IPCamera.IPCSetMDModeEx(0, true);
    document.IPCamera.IPCShowMDAreaEx(0, false);
    roiMaskArr[v][0] = 0;
    roiMaskArr[v][1] = 0;
    roiMaskArr[v][2] = 0;
    roiMaskArr[v][3] = 0;
    roiMaskArr[v][4] = 0;
    var jcpstr = "roicfg -act set -id "+v+" -enable 0  -left 0 -top 0 -right 0 -bottom 0";
    GetJCP({cmd: jcpstr,ParseJCP: function(jcpstr){
        if(jcpstr=="Error"){
            alert(IDC_MSGBOX_DELETEFAIL);
            window.focus();
        }else{
            $("#checkRoi").prop("checked",false);
            alert(IDC_MSGBOX_DELETEOK);
            window.focus();
        }
    }});
}
//-----------------------------------------ROI设置



var $w = $(window).width();
if($w < 1200){
   $("body").css("width",1200);
}
$(window).resize(function(){
    var $w = $(window).width();
    if($w < 1200){
       $("body").css("width",1200);
    }else{
       $("body").css("width",'99%');
    }
});

//音频设置
var involume, outvolume;
function showAudioInfo(){
    try{
        if(audioCfgArr === undefined){
            GetJCPList({cmd: "devaudioopt -act list", ParseJCP: function(jcpstr){
                audioCfgArr = JSON.parse(jcpstr.substring(0, jcpstr.length - 1));
            involume = $("#involume").slider({
                slide: function (event, ui)
                {
                    $("#tdInvolume").html(ui.value);
                },
                    min:parseInt(audioCfgArr["audioin"][0]["min"]),
                    max:parseInt(audioCfgArr["audioin"][0]["max"])
            });

            outvolume = $("#outvolume").slider({
                slide: function (event, ui)
                {
                    $("#tdOutvolume").html(ui.value);
                },
                    min:parseInt(audioCfgArr["audioout"][0]["min"]),
                    max:parseInt(audioCfgArr["audioout"][0]["max"])
            });


            }});
        }

        GetJCP({cmd: "audiocfg -act list", ParseJCP: function(jcpobj){
           
            involume.slider("value", parseInt(jcpobj.involume));
            $("#tdInvolume").html(jcpobj.involume);

            outvolume.slider("value", parseInt(jcpobj.outvolume));
            $("#tdOutvolume").html(jcpobj.outvolume);

            $("#codetype").val(jcpobj.codetype);
            $("#inputtype").val(jcpobj.inputtype);
            $("#amrbps").val(jcpobj.amrbps);
           
            $("input[name='rdAudioIN'][value=" + parseInt(jcpobj.inenable) + "]").attr("checked",true);
            SetDisableEnable(parseInt(jcpobj.inenable));
        }});
    }catch(E){ return E;}
}

function SetDisableEnable()
{
    var arg0 = arguments[0];
    if (typeof arg0 == "undefined")
    {
        return -1;
    }
    
    if (arg0 == 1)
    {
        $("#codetype").attr("disabled", false);
        $("#inputtype").attr("disabled", false);
        $("#divInvolume").hide();
        $("#divOutvolume").hide();
        $("#outvolume").show();
        $("#involume").show();
        $("#tdInvolume").show();
        $("#tdOutvolume").show();
        //DisableEnableBPS();
    }
    else if (arg0 == 0)
    {
        $("#codetype").attr("disabled", true);
        $("#inputtype").attr("disabled", true);
        $("#outvolume").hide();
        $("#involume").hide();
        var ti = $("#tdInvolume");
        var to = $("#tdOutvolume");
        $("#divInvolume").text(ti.text()).show();
        $("#divOutvolume").text(to.text()).show();
        ti.hide();
        to.hide();
    }
}

function SaveAudioSet(){
    var jcpstr = "audiocfg -act set  -inenable " + ($('input:radio[name="rdAudioIN"]:checked').val());
    jcpstr += " -involume " + (involume.slider("value")) + " -outvolume " + (outvolume.slider("value"));
    jcpstr += " -codetype " + parseInt($("#codetype").val());
     //+ " -amrbps " + $("#amrbps").val();
    jcpstr += " -inputtype " + parseInt($("#inputtype").val());
    GetJCP({cmd: jcpstr});
    
    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
    return;
}


function DefauleVolume(){
    involume.slider("value", parseInt(audioCfgArr["audioin"][0]["def"]));
    $("#tdInvolume").html(audioCfgArr["audioin"][0]["def"]);
    $("#divInvolume").text(audioCfgArr["audioin"][0]["def"]);
    outvolume.slider("value", parseInt(audioCfgArr["audioout"][0]["def"]));
    $("#tdOutvolume").html(audioCfgArr["audioout"][0]["def"]);
    $("#divOutvolume").text(audioCfgArr["audioout"][0]["def"]);
    window.focus();
}

//球机视频遮挡设置
//选择区域
function Areas()
{
    if (true == document.IPCamera.IPCSetMDModeEx(0, true))
    {
        document.IPCamera.IPCSetMDAreaRectEx(0,100,100,100,100);
        document.IPCamera.IPCShowMDAreaEx1(0,true,2,2);
        document.IPCamera.IPCSetMDAreaTitleEx(0, IDC_AREA);
    }
    window.focus();
}

//删除挡块
function DeleteRegion()
{
    var chns = GetCookieByKey("ChnIndex")+"/"+VencSize2Str(GetCookieByKey("StreamIndex"));
    document.IPCamera.IPCShowMDAreaEx(0, false);
    var indexs = $("#ShieldingIndex").find("option:selected").text();
    var jcpstr = "ptzmask -act del -chn " + chns + " -maskid "+indexs;
    GetJCP({cmd:jcpstr});
    window.focus();
}

//显示挡块
function Display()
{
    var chns = GetCookieByKey("ChnIndex")+"/"+VencSize2Str(GetCookieByKey("StreamIndex"));
    var indexs = $("#ShieldingIndex").find("option:selected").text();
    var jcpstr = "ptzmask -act show -chn " + chns + " -maskid " + indexs;
    GetJCP({cmd:jcpstr});
    window.focus();
}

//隐藏挡块
function HideRegion()
{
    var chns = GetCookieByKey("ChnIndex")+"/"+VencSize2Str(GetCookieByKey("StreamIndex"));
    document.IPCamera.IPCShowMDAreaEx(0, false);    
    var indexs = $("#ShieldingIndex").find("option:selected").text();
    var jcpstr = "ptzmask -act hide -chn " + chns + " -maskid " + indexs;
    GetJCP({cmd:jcpstr});
    window.focus();
}
//设置挡块
function SetRegion()
{
        var para = document.IPCamera.IPCGetVideoWndRect(0);
        if (isBlank(para))
        {
            throw 1;
        }

        ret = GetRtspKeyStr(para, "left");
        if (-2 == ret)
        {
            throw 10;
        }
        var rcLeft = parseInt(ret);

        ret = GetRtspKeyStr(para, "top");
        if (-2 == ret)
        {
            throw 11;
        }
        var rcTop = parseInt(ret);

        ret = GetRtspKeyStr(para, "right");
        if (-2 == ret)
        {
            throw 12;
        }
        var rcRight = parseInt(ret);

        ret = GetRtspKeyStr(para, "bottom");
        if (-2 == ret)
        {
            throw 13;
        }
        var rcBottom = parseInt(ret);
            
        if (rcRight <= rcLeft)
        {
            throw 21;
        }
            
        if (rcBottom <= rcTop)
        {
            throw 22;
        }

        var webShow = GetCookieByKey("ShowWeb");   
        var new_pelco = GetRtspKeyStr(webShow,"new_pelco");

        var chns = GetCookieByKey("ChnIndex")+"/"+VencSize2Str(GetCookieByKey("StreamIndex"));
        var chnsText = chns.split("/");
        var rectifyRate = 1;
        if("1080P" == chnsText[1] || "720P" == chnsText[1])
        {
            rectifyRate = 1.42; // (1920/1080) / (352/288)
        }
        
        //320,256(160,128) 转换单片机尺寸，新版本中直接发送选中区域的宽高。
        widths = Math.floor((rcRight - rcLeft)* 2/352 * 160);
        heights = Math.floor((rcBottom-rcTop) * 2/288 * 128/rectifyRate);

        if(1 == parseInt(new_pelco))
        {
            widths = Math.floor(rcRight - rcLeft);
            heights = Math.floor(rcBottom - rcTop);
        }

        document.IPCamera.IPCShowMDAreaEx(0, false);
        var indexs = $("#ShieldingIndex").find("option:selected").text();
        var jcpstr = "ptzmask -act set -chn " + chns + " -maskwidth "+ widths +" -maskheight "+ heights +" -maskid "+indexs;
        GetJCP({cmd:jcpstr});
        window.focus();
}

function ptz_click_bind(){
      //云台操作监听
      $("#direction_bar").bind("mouseleave",function(){
         $("#direction_bar").attr("src","/image/ptz/ptz.png");
      });
      $("#direction_bar_map").bind("mouseleave",function(){
         $("#direction_bar").attr("src","/image/ptz/ptz.png");
      });
      $("#direction_bar_map area").click(function(event) {
        return event.preventDefault();
      });
      $("#direction_bar_map area[data-direction=center]").bind("mouseenter",function(){
        $("#direction_bar").attr("src","/image/ptz/ptz_center.png");
      }).bind("mousedown", function() {
        ptzs("ptz -chn " + chns + " -cmd 9 -speed  31");
      }).bind("mouseup", function() {
        ptzs("ptz -chn " + chns + " -cmd 10 -speed  31");
      }).bind("click",function() {
        ptzs("pelcod20ctrl -type 11 -cmd 14 -data2 1");
      });
      $("#direction_bar_map area[data-direction=up]").bind("mouseenter",function(){
        $("#direction_bar").attr("src","/image/ptz/ptz_up.png");
      }).bind("mousedown", function() {
         ptzs("pelcod20ctrl -type 1 -cmd 1 -data2 31");
      }).bind("mouseup", function() {
         ptzs("pelcod20ctrl -type 1 -cmd 9");
      });
      $("#direction_bar_map area[data-direction=down]").bind("mouseenter",function(){
        $("#direction_bar").attr("src","/image/ptz/ptz_down.png");
      }).bind("mousedown", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 2 -data2 31");
      }).bind("mouseup", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 9");
      });
      $("#direction_bar_map area[data-direction=left]").bind("mouseenter",function(){
        $("#direction_bar").attr("src","/image/ptz/ptz_left.png");
      }).bind("mousedown", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 3 -data1 31");
      }).bind("mouseup", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 9");
      });
      $("#direction_bar_map area[data-direction=right]").bind("mouseenter",function(){
        $("#direction_bar").attr("src","/image/ptz/ptz_right.png");
      }).bind("mousedown", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 4 -data1 31");
      }).bind("mouseup", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 9");
      });
      $("#direction_bar_map area[data-direction=center]").click(function() {
        ptzs("pelcod20ctrl -type 11 -cmd 14 -data2 1");
      });
      $("#direction_bar_map area[data-direction=right_up]").bind("mouseenter",function(){
        $("#direction_bar").attr("src","/image/ptz/ptz_up_right.png");
      }).bind("mousedown", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 5 -data1 31 -data2 31");
      }).bind("mouseup", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 9");
      });
      $("#direction_bar_map area[data-direction=right_down]").bind("mouseenter",function(){
        $("#direction_bar").attr("src","/image/ptz/ptz_right_down.png");
      }).bind("mousedown", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 6 -data1 31 -data2 31");
      }).bind("mouseup", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 9");
      });
      $("#direction_bar_map area[data-direction=left_up]").bind("mouseenter",function(){
        $("#direction_bar").attr("src","/image/ptz/ptz_left_up.png");
      }).bind("mousedown", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 7 -data1 31 -data2 31");
      }).bind("mouseup", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 9");
      });
      $("#direction_bar_map area[data-direction=left_down]").bind("mouseenter",function(){
        $("#direction_bar").attr("src","/image/ptz/ptz_left_down.png");
      }).bind("mousedown", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 8 -data1 31 -data2 31");
      }).bind("mouseup", function() {
        ptzs("pelcod20ctrl -type 1 -cmd 9");
      });
}

function ptzs(str){
    GetJCP({cmd: str});
}
