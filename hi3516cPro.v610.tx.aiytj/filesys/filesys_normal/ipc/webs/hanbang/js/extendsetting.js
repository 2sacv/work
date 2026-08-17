$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        show3dNoise();
        showEncodeSetting();
        showHikvisonNVR();
    }
})

var strengthSlider;
function show3dNoise(){
    strengthSlider = new SliderModules({
        targetId: "StrengthSlider",
        min: 0,
        max: 100
      }); 
      strengthSlider.create();
      strengthSlider.onchange = function () {
          $('#tdStrengthValue').text(strengthSlider.getValue());
      };
   
    GetJCP({cmd: "denoisecfg -act list", ParseJCP: function(jcpstr){
            $("input[name='denoise'][value=" + parseInt(jcpstr.enable) + "]").attr("checked",true);
            strengthSlider.wsetValue(parseInt(jcpstr.mode));
            $("#tdStrengthValue").html(jcpstr.mode);
    }});
}

var encodeSettingArr = [[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0]];
function showEncodeSetting(){
    //删除不匹配的码流尺寸
    var maxheight = GetCookieByKey("maxheight");
    if(maxheight == '1440'){
        $("#selEncodeSize option[value='13']").remove(); //5M
    }else if(maxheight == '1080'){
        $("#selEncodeSize option[value='13']").remove(); //5M
        $("#selEncodeSize option[value='12']").remove(); //4M
        $("#selEncodeSize option[value='9']").remove(); //3M
    }else if(maxheight == '960'){
        $("#selEncodeSize option[value='5']").remove(); //1080P
        $("#selEncodeSize option[value='9']").remove(); //3M
        $("#selEncodeSize option[value='13']").remove(); //5M
        $("#selEncodeSize option[value='12']").remove(); //4M
    }else if(maxheight == '720'){
        $("#selEncodeSize option[value='8']").remove(); //960p
        $("#selEncodeSize option[value='5']").remove(); //1080P
        $("#selEncodeSize option[value='9']").remove(); //3M
        $("#selEncodeSize option[value='13']").remove(); //5M
        $("#selEncodeSize option[value='12']").remove(); //4M
    }else if(maxheight == '1296' || maxheight == '1536'){
        $("#selEncodeSize option[value='13']").remove(); //5M
        $("#selEncodeSize option[value='12']").remove(); //4M
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

    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
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

    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
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