var masterCfgArr;
var slaveCfgArr;  

$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
       showStream();
    }
})

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

function ParseDeviceOptCfg(jcpstr) {
	try {
		var streamJson = JSON.parse(jcpstr.substring(0, jcpstr.length - 1));
		x264Rate = parseFloat(streamJson["x264"]);
		masterCfgArr = streamJson["master"] || [];
		slaveCfgArr = streamJson["slave"] || [];
		mater_html = '';
		slave_html = '';
		for (var i = 0; i < masterCfgArr.length; ++i) {
			mater_html += '<option value="' + masterCfgArr[i]["id"] + '">' + masterCfgArr[i]["name"] + '</option>';
		}
		for (var i = 0; i < slaveCfgArr.length; ++i) {
			slave_html += '<option value="' + slaveCfgArr[i]["id"] + '">' + slaveCfgArr[i]["name"] + '</option>';
		}
	}
	catch (E){}
}

function ParseStreamHeightCfg(jcpobj){
    if(jcpobj != 'Error'){
		if (jcpobj.maxheight == '2160') {
          //  $("#selStreamMaster option[value='15']").remove(); //8M
    	}else if (jcpobj.maxheight == '1920' || jcpobj.maxheight == '1620') {
			$("#selStreamMaster option[value='15']").remove(); //8M
		} else if(jcpobj.maxheight == '1440'){ //4M
            $("#selStreamMaster option[value='15']").remove(); //8M
            $("#selStreamMaster option[value='13']").remove(); //5M
        }else if(jcpobj.maxheight == '1080'){
            $("#selStreamMaster option[value='15']").remove(); //8M
            $("#selStreamMaster option[value='13']").remove(); //5M
            $("#selStreamMaster option[value='12']").remove(); //4M
            $("#selStreamMaster option[value='9']").remove(); //3M
        }else if(jcpobj.maxheight == '960'){
            $("#selStreamMaster option[value='15']").remove(); //8M
            $("#selStreamMaster option[value='13']").remove(); //5M
            $("#selStreamMaster option[value='12']").remove(); //4M
            $("#selStreamMaster option[value='9']").remove(); //3M
            $("#selStreamMaster option[value='5']").remove(); //1080P
        }else if(jcpobj.maxheight == '720'){
            $("#selStreamMaster option[value='15']").remove(); //8M
            $("#selStreamMaster option[value='13']").remove(); //5M
            $("#selStreamMaster option[value='12']").remove(); //4M
            $("#selStreamMaster option[value='9']").remove(); //3M
            $("#selStreamMaster option[value='5']").remove(); //1080P
            $("#selStreamMaster option[value='8']").remove(); //960p
        }else if(jcpobj.maxheight == '1296' || jcpobj.maxheight == '1536'){//3M
			if (jcpobj.cpu == 'SSC327E') {
				$("#selStreamMaster option[value='15']").remove(); //8M
			} else {
                $("#selStreamMaster option[value='15']").remove(); //8M
                $("#selStreamMaster option[value='13']").remove(); //5M
                $("#selStreamMaster option[value='12']").remove(); //4M
            }
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
/*
        if(parseInt(masterObj.enable)==0){
            DisableMaster(1);
        }

        if(parseInt(slaveObj.enable)==0){
            DisableSlave(1);
        }
        */
		
        ChgStream(0,3); // 3 的值是为了防止重置默认值
        ChgStream(1,3);
    }catch(E){}
}

var frmrateTip = ''; //帧率提示范围
var frmrateMasterTip = ''; //主码流帧率提示范围

function getSelectMasterObj() {
	var m = $("#selStreamMaster").val();
	for (var i = 0; i < masterCfgArr.length; ++i)
    {
		if (m == masterCfgArr[i]["id"])
		{
			return masterCfgArr[i];
		}
    }
}

function getSelectSlaveObj() {
	var s = $("#selStreamSlave").val();
    for (var i = 0; i < slaveCfgArr.length; ++i)
    {
		if (s == slaveCfgArr[i]["id"])
		{
			return slaveCfgArr[i];
		}
    }
}

//保存码流设置
function SaveStream(){
    var rdmaster = 1;//parseInt($('input:radio[name="rdMaster"]:checked').val());
    var rdslave = 1;//parseInt($('input:radio[name="rdSlave"]:checked').val());

    if(rdmaster == 0 && rdslave == 0){
        alert(IDC_CHANNEL_NUM_MIN);
        window.focus();
        return;
    }

    //分辨率和码率验证
	var masterObj = getSelectMasterObj();
	var slaveObj = getSelectSlaveObj();

    var mRate = $("#frmrateMaster").val();
    var mIntr = $("#frmintrMaster").val();
    var sRate = $("#frmrateSlave").val();
    var sIntr = $("#frmintrSlave").val();
    var ms = $("#selStreamMaster").val();
    var ss = $("#selStreamSlave").val();
    var mb = $("#bitrateMaster").val();
    var sb = $("#bitrateSlave").val();

	if (mRate > masterObj["fps_max"] || mRate < 1) {
		alert(IDC_RATE_FAIL_START + "(1~" + masterObj["fps_max"] + ")" + IDC_RATE_FAIL_END);
        window.focus();
        return;
	}
	
	if (sRate > slaveObj["fps_max"] || sRate < 1) {
		alert(IDC_RATE_FAIL_START + "(1~" + slaveObj["fps_max"] + ")" + IDC_RATE_FAIL_END);
        window.focus();
        return;
	}
	
    if (mIntr > 360 || sIntr > 360 || mIntr == 0 || sIntr == 0) {
        alert(IDC_INTERVAL_FAIL);
        window.focus();
        return;
    }

    if (ms == 15 && (mb > masterObj["bps_max"] || mb < masterObj["bps_min"])) {
        alert(IDC_BPS_FAIL_8M);
        window.focus();
        return;
    }
    
    if (ms == 13 && (mb > masterObj["bps_max"] || mb < masterObj["bps_min"])) {
        alert(IDC_BPS_FAIL_5M);
        window.focus();
        return;
    }

    if (ms == 12 && (mb > masterObj["bps_max"] || mb < masterObj["bps_min"])) {
        alert(IDC_BPS_FAIL_4M);
        window.focus();
        return;
    }

    if (ms == 9 && (mb > masterObj["bps_max"] || mb < masterObj["bps_min"])) {
        alert(IDC_BPS_FAIL_3M);
        window.focus();
        return;
    }

    if (ms == 5 && (mb > masterObj["bps_max"] || mb < masterObj["bps_min"])) {
        alert(IDC_BPS_FAIL_1080P);
        window.focus();
        return;
    }

    if (ms == 8 && (mb > masterObj["bps_max"] || mb < masterObj["bps_min"])) {
        alert(IDC_BPS_FAIL_960p);
        window.focus();
        return;
    }

    if (ms == 3 && (mb > masterObj["bps_max"] || mb < masterObj["bps_min"])) {
        alert(IDC_BPS_FAIL_720P);
        window.focus();
        return;
    }
    
    if (ms == 7 && (mb > masterObj["bps_max"] || mb < masterObj["bps_min"])) {
        alert(IDC_BPS_FAIL_VGA);
        window.focus();
        return;
    }

    if (ms == 2 && (mb > masterObj["bps_max"] || mb < masterObj["bps_min"])) {
        alert(IDC_BPS_FAIL_D1);
        window.focus();
        return;
    }

    if (sb > 1024 || sb < 32) {
        alert(IDC_BPS_FAIL_SLAVE);
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
	
	
	//如果设置码率过小，弹出提示框“码率设置过小会导致画质不佳，确定要保存设置”，点击“确定”保存，点击“取消”不保存
	if(mb < 1024 || sb < 512){
        //parent.paramFailTip(IDC_BPS_TOO_SMALL);
		
		if(window.confirm(IDC_BPS_TOO_SMALL)){
			doJcp(jcpstr, rdmaster, rdslave);
		}
	}else{
		doJcp(jcpstr, rdmaster, rdslave);
	}
    
}

function doJcp(jcpstr, rdmaster, rdslave){
	GetJCPList({cmd: jcpstr, ParseJCP: function(result){
        if(result != "Error")
        {
            //var stream_size = rdmaster == 1 ? "stream1" : "stream2";
            //主码流修改为不能关闭，所以除主页视频外永远显示主码流
            Set_cookie("stream", "stream1");
            Set_cookie("master_enb",parseInt(rdmaster));
            Set_cookie("master_stream",$("#selStreamMaster").val())
            Set_cookie("slave_enb",parseInt(rdslave));
            Set_cookie("slave_stream",$("#selStreamSlave").val())
            parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
        }
        else
        {
            parent.paramFailTip(IDC_MSGBOX_SAVEFAIL);
        }
        window.focus();
    }});
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
        parent.paramFailTip(IDC_CHANNEL_NUM_MIN);
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

function getSelectSlaveObj() {
	var s = $("#selStreamSlave").val();
    for (var i = 0; i < slaveCfgArr.length; ++i)
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

function setSlaveObj(vesize) {
    $("#selStreamSlave").val(vesize);
	var m = $("#selStreamSlave").val();
	for (var i = 0; i < masterCfgArr.length; ++i)
    {
		if (m == masterCfgArr[i]["id"])
		{
            $("#bitrateSlave").val(masterObj[i]["bps_def"]);
            $("#frmrateSlave").val(masterObj[i]["fps_def"]);
            $("#frmintrSlave").val(masterObj[i]["gop_def"]);
		}
    }
}

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
            if($("#selStreamSlave").val()==2){
                setSlaveObj(7);
            }
            //删从码流的D1
            $("#selStreamSlave > option").each(function (){
                if (parseInt($(this).attr("value")) == 2)
                {
                    $(this).remove();
                }
            });

            //如果vga删除了，则恢复
            var vExist = 0;
            $("#selStreamSlave > option").each(function (){
                if (parseInt($(this).attr("value")) == 7)
                {
                    vExist ++;
                }
            });

            if (vExist == 0)
            {
                $("#selStreamSlave").append('<option value="7">' + 'VGA' + '</option>');
            }
        }else if(7 == parseInt(s)){//当主码流选择VGA，从码流不能出现VGA
            if($("#selStreamSlave").val()==2 || $("#selStreamSlave").val()==7){
                setSlaveObj(1);
            }

            //删从码流的D1 VGA
            $("#selStreamSlave > option").each(function (){
                if (parseInt($(this).attr("value")) == 2 || parseInt($(this).attr("value")) == 7)
                {
                    $(this).remove();
                }
            });
            /*
        }else {
            //如果vga,d1删除了，则恢复
            var d1Exist=0,vExist=0;
            $("#selStreamSlave > option").each(function (){
                if (parseInt($(this).attr("value")) == 2)
                {
                    d1Exist ++;
                }
                if (parseInt($(this).attr("value")) == 7)
                {
                    vExist ++;
                }
            });
            if (d1Exist == 0)
            {
                $("#selStreamSlave").append('<option value="2">' + 'D1' + '</option>');
            }
            if (vExist == 0)
            {
                $("#selStreamSlave").append('<option value="7">' + 'VGA' + '</option>');
            }
            */
        }
    } else if(chn == 1) {
        var s1 = $("#selStreamSlave").val();
        if(flag != 3){
		    changeSlaveStreamDefValue();
        }
		changeSlaveStreamTip();
    }
}
