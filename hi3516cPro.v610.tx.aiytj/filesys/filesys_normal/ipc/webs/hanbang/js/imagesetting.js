$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        if($.cookie("graintype") == 0){
          $("#divExtend").hide(); //隐藏扩展字幕
        }
        if( $.cookie("has_dome") == 1){ //球机
            $("#divVideoControl").addClass('spanTabActive');
            show = 1;
        }else{
            $("#divColorParamSet").show();
            $("#divVideoControl").show();
            $("#divAdvanceSet").show();
            $("#tbColorParamSet").show();
        }       
        showVideo();
        initVideoSetting();
    }
})

var show = 0;
var wndWidth = 350; //视频宽度
var wndHeight = 250; //视频高度
var veWidth=1920, veHeight=1080; //用插件获取到的尺寸
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

function showTab(s){
  if(show == s){
    return;
  }
  show = s;
  if(s == 0){
    $("#divVideoControl").removeClass('spanTabActive');
    $("#divAdvanceSet").removeClass('spanTabActive');
    $("#divColorParamSet").addClass('spanTabActive');
    $("#tbVideoControl").hide();
    $("#tbAdvanceSet").hide();
    $("#tbColorParamSet").show();
  }else if(s == 1){
    $("#divColorParamSet").removeClass('spanTabActive');
    $("#divAdvanceSet").removeClass('spanTabActive');
    $("#divVideoControl").addClass('spanTabActive');
    $("#tbColorParamSet").hide();
    $("#tbAdvanceSet").hide();
    $("#tbVideoControl").show();
  }else{
    $("#divVideoControl").removeClass('spanTabActive');
    $("#divColorParamSet").removeClass('spanTabActive');
    $("#divAdvanceSet").addClass('spanTabActive');
    $("#tbVideoControl").hide();
    $("#tbColorParamSet").hide();
    $("#tbAdvanceSet").show();
  }
}

var nightluma, bright, contrast, saturation, gain,contrastAgain,highLightSuppress,sharpness;
var g_hdr = 0; //宽动态
//图像设定初始化
function initVideoSetting(){
    try
    {
        nightluma = new SliderModules({
                targetId: "nightluma",
                min: 1,
                max: 100
              }); 
              nightluma.create();
              nightluma.onchange = function () {
                  $('#tdNightluma').text(nightluma.getValue());
                  GetJCP({cmd: "vicfg -act set  -nightluma " + nightluma.getValue()});
              };

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

        /*contrastAgain = new SliderModules({
                targetId: "contrastAgain",
                min: 0,
                max: 6
              }); 
              contrastAgain.create();
              contrastAgain.onchange = function () {
                  $('#tdContrastAgain').text(contrastAgain.getValue());
                  GetJCP({cmd: "vicfg -act set  -stren 1 -brightlevel " + contrastAgain.getValue()});
              };*/
        
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

        gain = new SliderModules({
                targetId: "gain",
                min: 1,
                max: 255
              }); 
              gain.create();
              gain.onchange = function () {
                  $('#tdGain').text(gain.getValue());
                  GetJCP({cmd: "vicfg -act set -gain " + gain.getValue()});
              };
        highLightSuppress = new SliderModules({
            targetId: "highLightSuppress",
            min: 0,
            max: 100
          }); 
          highLightSuppress.create();
          highLightSuppress.onchange = function () {
              $('#tdHighLightSuppress').text(highLightSuppress.getValue());
              GetJCP({cmd: "vicfg -act set -suppress " + highLightSuppress.getValue()});
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

        GetJCP({cmd: "aeawbblccfg -act list", ParseJCP: Parseaeawb});
    }
    catch(E){return E;}
}

//初始化图像设定-颜色参数信息
function ParseVICfg(jcpstr)
{
    try
    {
        nightluma.wsetValue(parseInt(jcpstr.nightluma));
        bright.wsetValue(parseInt(jcpstr.bright));
        contrast.wsetValue(parseInt(jcpstr.contrast));
        saturation.wsetValue(parseInt(jcpstr.saturation));
        gain.wsetValue(parseInt(jcpstr.gain));
        //contrastAgain.wsetValue(parseInt(jcpstr.brightlevel));
        sharpness.wsetValue(parseInt(jcpstr.sharpness));
        highLightSuppress.wsetValue(parseInt(jcpstr.suppress));

        $('#tdNightluma').text(jcpstr.nightluma);
        $('#tdBright').text(jcpstr.bright);
        $('#tdContrast').text(jcpstr.contrast);
        //$('#tdContrastAgain').text(jcpstr.brightlevel);
        $('#tdSaturation').text(jcpstr.saturation);
        $('#tdGain').text(jcpstr.gain);
        $('#tdSharpness').text(jcpstr.sharpness);
        $('#tdHighLightSuppress').text(jcpstr.suppress);


        /*if(parseInt(jcpstr.stren) == 1){
            $("#cbContrastAgain").prop("checked",true);
        }else{
            $("#cbContrastAgain").prop("checked",false);
        }
        checkContrastAgain();*/
        $("input[name='lampchk'][value=" + parseInt(jcpstr.lampfrequency) + "]").attr("checked",true);

        if(jcpstr.reverse < 0 || jcpstr.reverse > 4){
            $("input[name='videoreverse'][value=0]").attr("checked",true);
        }else{
            $("input[name='videoreverse'][value=" + parseInt(jcpstr.reverse) + "]").attr("checked",true);
        }
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
        $("#lowlightenhance").val(jcpGet.lowlightenhance);
		    $("#nightfacemode").val(jcpGet.nightfacemode);
        $("#aewinweight").val(jcpGet.aewinweight);
        //$("#strengthenToMist").val(jcpGet.defogenhance);
    }
    catch(e){ return e; }
}
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
        parent.paramFailTip(IDC_DIEJIA);
        window.focus();
        $("#redgain").attr("value",128);
    }
}

function blurgainBlur(){
    var blurgains = $("#blurgain").val();
    if(isNaN(blurgains) || blurgains < 1 || blurgains > 255) 
    {
        parent.paramFailTip(IDC_DIEJIA);
        window.focus();
        $("#blurgain").attr("value",128);
    }
}

function lowlightenhanceBlur(){
    var lowlightenhance = $("#lowlightenhance").val();
    if(isNaN(lowlightenhance) || lowlightenhance < 0 || lowlightenhance > 100) 
    {
        parent.paramFailTip(IDC_DIEJIA);
        window.focus();
        $("#lowlightenhance").attr("value",50);
    }
}
function strengthenToMistBlur(){
    var strengthenToMist = $("#strengthenToMist").val();
    if(isNaN(strengthenToMist) || strengthenToMist < 0 || strengthenToMist > 100) 
    {
        parent.paramFailTip(IDC_DIEJIA);
        window.focus();
        $("#strengthenToMist").attr("value",50);
    }
}
function AWBSave(){
    var redgains = $("#redgain").val();
    var blurgains = $("#blurgain").val();
    var AESelect = $("#AESelect").val();
    var AWBSelect = $("#AWBSelect").val();
	  var nightfacemode = $("#nightfacemode").val();
    var lowlightenhance = $("#lowlightenhance").val();
    //var defogenhance = $("#strengthenToMist").val();
    //var aewinweight = $("#aewinweight").val();

    if(isNaN(redgains) || redgains < 0 || redgains > 255)
    {
        parent.paramFailTip(IDC_DIEJIA);
        window.focus();
        return;
    }
    if(isNaN(blurgains) || blurgains < 0 || blurgains > 255) 
    {
        parent.paramFailTip(IDC_DIEJIA);
        window.focus();
        return;
    }
    if(isNaN(lowlightenhance) || lowlightenhance < 0 || lowlightenhance > 100) 
    {
        parent.paramFailTip(IDC_DIEJIA);
        window.focus();
        return;
    }
/*
    if(isNaN(defogenhance) || defogenhance < 0 || defogenhance > 100) 
    {
        parent.paramFailTip(IDC_DIEJIA);
        window.focus();
        return;
    }
*/

    var jcpSet = "aeawbblccfg -act set -aeCtrlMode " + AESelect + " -awbCtrlMode " + AWBSelect + " -redGain " + redgains + " -blueGain " + blurgains + " -nightfacemode " + nightfacemode + " -lowlightenhance " + lowlightenhance;
    GetJCP({cmd: jcpSet});
   // GetJCP({cmd: "lenscs -act set -enable "+ ($("#shadowCorrection").is(":checked")==true?1:0)});
    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();
}

var defColorValue = 128;
var defNightluma = 50;
function DefaultColor()
{
    nightluma.wsetValue(defNightluma);
    bright.wsetValue(defColorValue);
    contrast.wsetValue(defColorValue);
    saturation.wsetValue(defColorValue);
    gain.wsetValue(defColorValue);
    //contrastAgain.wsetValue(3);

    sharpness.wsetValue(defColorValue);
    highLightSuppress.wsetValue(50);

    $('#tdNightluma').text(defNightluma);
    $('#tdBright').text(defColorValue);
    $('#tdContrast').text(defColorValue);
    //$('#tdContrastAgain').text(3);
    $('#tdSaturation').text(defColorValue);
    $('#tdGain').text(defColorValue);
    $('#tdSharpness').text(defColorValue);
    $('#tdHighLightSuppress').text(50);

   
    $("input[name='lampchk'][value=1]").attr("checked", true);
    var jcpstr = "vicfg -act set -nightluma " + defNightluma + " -bright " + defColorValue + " -contrast " + defColorValue + 
                                " -saturation " + defColorValue + " -sharpness " + defColorValue +" -gain " + defColorValue+ " -suppress 50 ";
    jcpstr += " -lampfrequency 1 -brightlevel 3";
    GetJCP({cmd: jcpstr});
    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();
}


function lamchkSave()
{
    var l = $('input:radio[name="lampchk"]:checked').val();
    GetJCP({cmd: "vicfg -act set -lampfrequency " + l});
    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();
}

function PreviewReverse(){
    var i = parseInt($('input:radio[name="videoreverse"]:checked').val());
    GetJCPList({cmd: "vicfg -act set -reverse " + i});
}
