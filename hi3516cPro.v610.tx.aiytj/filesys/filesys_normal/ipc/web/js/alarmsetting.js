var ptz_alarm_in = 0; //是否有报警输入，大于0有
var ptz_alarm_out = 0; //是否有报警输出，大于0有
var has_dome = 0; //是否是球机
var aiLinkage = [[],[],[],[]];//报警输入联动参数,4输入
var aiLinkageChg = new Array(0, 0, 0, 0);//报警输入配置参数是否改变 0未改变, 1改变
var aiChannelTime = [[],[],[],[]];//报警输入通道时间布防
$(function(){
    ptz_alarm_in =  $.cookie("has_alarmin");
    ptz_alarm_out = $.cookie("has_alarmout");
    has_dome = $.cookie("has_dome");
    var lf = $.cookie("loginflag_"+g_hostname);
    var en_follow = $.cookie("en_follow");
    
    if (1 == parseInt(en_follow)){
        $("#liMF").show();
    }
    
    GetJCP({cmd: "ioalarmcfg -act list", ParseJCP: function(jcpobj){
        if (jcpobj.show == 0) {
            $("#liIO").hide();
        } else {
            $("#liIO").show();
        }
    }});

    if (null === lf || typeof(lf) =='undefined' || 0 > parseInt(lf))
    {
       self.location.href = "/login.asp";
    }else{
        if(parseInt(ptz_alarm_in) <= 0){
            $("#liAI").hide();
        }

        if($.cookie("graintype") == 0){
            $("#liAI").hide();
        }
        if( parseInt(has_dome)==1 ||  parseInt(ptz_alarm_out) <= 0 ){
            $("#liAO").hide();
        }
        if(parseInt($.cookie("hdetect")) == 1) {
            $("#liPeople").show();
        } else {
            $("#liPeople").hide();
        if(parseInt($.cookie("cartect")) == 1) {
            $("#liCarDetect").show();
            $("#CarLink").show();
        } else {
            $("#liCarDetect").hide();
            $("#CarLink").hide();
        }
        }

         $('#tbAlarmLink tr').find('td:eq(3)').hide();
        
        initMD();
        $("#tabs").tabs();
        $("select").bind("change",function(){
             window.focus();
        }); 

        initInputChannel();
    }
});

//读取输入通道，初始化相关信息
function initInputChannel(){
    //模拟报警类别
    var _simulateAlarmTypeHtml = '';
    var _inputChannelHtml = '';
    var _allChannel_html = '';

    _simulateAlarmTypeHtml += IDC_ALARM_TYPE;
    _simulateAlarmTypeHtml += '<select id="alarmType" class="sysinput2">';
    var _html = '';
    if(parseInt(ptz_alarm_in) > 0){
       for(var i=0;i<parseInt(ptz_alarm_in);i++){
            _simulateAlarmTypeHtml += '<option value="'+(80+i)+'">'+IDC_ALARM_TYPE_AI_CHN+(i+1)+'</option>';
            _inputChannelHtml += '<option value="'+i+'">'+(i+1)+'</option>';

            _allChannel_html += '<input type="checkbox" id="copyChannel'+i+'"';
            if(i==0){
                _allChannel_html += ' disabled="disabled" checked="checked" ';
            }
            _allChannel_html += ' onclick="checkAllChanelIsCheck()"/>';
            _allChannel_html += '<label for="copyChannel'+i+'">'+IDC_CHANNEL+(i+1)+'</label>&nbsp;&nbsp;';
        }
    }
    $("#aisel").html(_inputChannelHtml);
    $("#inputChannelSel").html(_inputChannelHtml);

      // _simulateAlarmTypeHtml += '<option value="1">'+IDC_MD_LINKAGE+'</option>';
    _simulateAlarmTypeHtml += '<option value="2">'+IDC_MD_VGLINE+'</option>';
    _simulateAlarmTypeHtml += '<option value="3">'+IDC_MD_VGRECT+'</option>';
    _simulateAlarmTypeHtml += '<option value="13">'+IDC_HUMAN_DETECTION+'</option>';
    _simulateAlarmTypeHtml += '<option value="20">'+IDC_PEOPLE_CAR_DETECT+'</option>';
    _simulateAlarmTypeHtml += '</select>&nbsp;&nbsp;&nbsp;&nbsp;';

    _simulateAlarmTypeHtml += '<label for="enSimAL">'+IDC_SIMULATE_FILTER_RULE+'</label>';
    _simulateAlarmTypeHtml += '<input type="checkbox" id="enSimAL"/>&nbsp;&nbsp;&nbsp;&nbsp;';
    _simulateAlarmTypeHtml += '<button style="width:85px;margin-left:0px;margin-top:0px;" class="btn btn-inverse btn-black index_btn"  onclick="SimulateAlarm();">'+IDC_SIMULATE+'</button>';
    _simulateAlarmTypeHtml += '&nbsp;&nbsp;&nbsp;&nbsp;<font id="simstatus" ></font>';

    $("#tdSimulate").html(_simulateAlarmTypeHtml);

    $("#spanCopy").html(_allChannel_html);

}


//复制通道
function copyChannelAll(){
    var flag = $("#copyChannel").is(":checked");
    var ioSel = $("#inputChannelSel").val();
    for(var i=0;i<parseInt(ptz_alarm_in);i++){
        if(i != ioSel){
            $("#copyChannel"+i).prop("checked",!!flag);
        }
    }
}

function changeInputChannel(){
    var ioSel = $("#inputChannelSel").val();
    
    $("#enableIOTime").prop("checked", aiChannelTime[ioSel][0]==1?true:false);
    $("#ai_time_protection").selectTime('setData',aiChannelTime[ioSel][1]);

    for(var i=0;i<parseInt(ptz_alarm_in);i++){
        if(i == ioSel){
            $("#copyChannel"+i).attr("disabled",true).prop("checked",true);
        }else{
            $("#copyChannel"+i).attr("disabled",false).prop("checked",false);
        }
    }
    $("#copyChannel").prop("checked",false);
}

function checkAllChanelIsCheck(){
    var sum = 0;
    for(var i=0;i<parseInt(ptz_alarm_in);i++){
        sum += $("#copyChannel"+i).is(":checked")?1:0;
    }
    if(sum == ptz_alarm_in){
        $("#copyChannel").prop("checked",true);
    }else{
        $("#copyChannel").prop("checked",false);
    }
}


function initVL(){
    if(initVideoShow>0){
        initVideoShow=0;
        document.IPCamera.IPCStopPreview(0);
        $("#motion_ipcamer").html('');
    }
    g_curr_tab = "IDC_VL_TIME_STRATEGY";
    $("#vl_time_protection").selectTime();
    GetJCP({cmd: "vlcfg -act list", ParseJCP: ParseVLCfg});
}

function initAI(){
    if(initVideoShow>0){
        initVideoShow=0;
        document.IPCamera.IPCStopPreview(0);
        $("#motion_ipcamer").html('');
    }
    g_curr_tab = "JALARM_TYPE_AI";
    $("#ai_time_protection").selectTime();
    getAlarmInByJcp();
}

function initLinkSet(){
    if(initVideoShow>0){
        initVideoShow=0;
        document.IPCamera.IPCStopPreview(0);
        $("#motion_ipcamer").html('');
    }
    g_curr_tab = "IDC_ALARMLINKATTR";
    GetJCP({cmd: "alarmoutcfg -act list ", ParseJCP: ParseAlarmoutCfg});
}

function initVM(){
    if(initVideoShow>0){
        initVideoShow=0;
        document.IPCamera.IPCStopPreview(0);
        $("#motion_ipcamer").html('');
    }
    g_curr_tab = "IDC_VM_TIME_STRATEGY";
    $("#vm_time_protection").selectTime();
    initVmtype();
    GetJCP({cmd: "vmaskalarmcfg -act list", ParseJCP: ParseVMCfg});
}

function ParseVLCfg(jcpobj){
    try{
        var flag = (parseInt(jcpobj.enable)==1)?true:false;
        $("#losschk").prop("checked",flag);
        $("#vl_time_protection").selectTime('setData',jcpobj.timestrategy)
    }catch(e){}
}

function ParseAlarmoutCfg(jcpobj){
    try
    {
        $("input[name='ao0status'][value=" + parseInt(jcpobj.ao0) + "]").attr("checked",true);
        $("input[name='ao1status'][value=" + parseInt(jcpobj.ao1) + "]").attr("checked",true);
        $("input[name='ao0level'][value=" + parseInt(jcpobj.ao0alarm) + "]").attr("checked",true);
        $("input[name='ao1level'][value=" + parseInt(jcpobj.ao1alarm) + "]").attr("checked",true);
        $("#aoholdtime").val(jcpobj.holdtime);
    }catch(e){
    }
}

function SaveLinkSet(){
    var aoholdtime = $("#aoholdtime").val();

    if (isBlank(aoholdtime) || aoholdtime > 36000 || 1 > aoholdtime)
    {
        alert(IDC_AO_HOLDTIME_MSG + "1~36000");//报警保持时间
        window.focus();
        return false;
    }

    var jcpstr = "alarmoutcfg -act set " +  " -ao0 " + parseInt($('input:radio[name="ao0status"]:checked').val())
                + " -ao1 " + parseInt($('input:radio[name="ao1status"]:checked').val()) + " -ao0alarm " + 
                parseInt($('input:radio[name="ao0level"]:checked').val()) + " -ao1alarm " + parseInt($('input:radio[name="ao1level"]:checked').val())
                + " -holdtime " + aoholdtime;
    GetJCP({cmd: jcpstr});

    alert(IDC_MSGBOX_SAVEOK);
    window.focus();
}


function getAlarmInByJcp(){
    try{
        GetJCP({cmd: "alarmincfg -act list", ParseJCP: function(jcpobj){
            if(jcpobj != 'Error'){
                for(var i=0;i<parseInt(ptz_alarm_in);i++){
                    aiChannelTime[i][0] = jcpobj["ai"+i+"en"];
                    aiChannelTime[i][1] = jcpobj["timestrategy"+i];
                }

                $("#enableIOTime").prop("checked",jcpobj.ai0en==1?true:false);
                $("#ai_time_protection").selectTime('setData',aiChannelTime[0][1]);
            }
        }});
    }catch(e){}
}

function SaveAlarmIn(){
     var ai = $("#inputChannelSel").val();
     var aiEnable = $("#enableIOTime").is(":checked")?1:0;
     var timestrategy = $("#ai_time_protection").selectTime('getData');

     var jspstr =  "alarmincfg -act set ";

     for(var i=0;i<parseInt(ptz_alarm_in);i++){
        if($("#copyChannel"+i).is(":checked")){
            aiChannelTime[i][0] = aiEnable;
            aiChannelTime[i][1] = timestrategy;
        }

        jspstr += " -ai"+i+"en " + aiChannelTime[i][0] + " -timestrategy"+i+" "+aiChannelTime[i][1];
    }
 
    GetJCP({cmd:jspstr , ParseJCP: function(result){
            var prompt = IDC_SAVE + IDC_SUCCESS;
            if(result == "Error")
            {
                prompt = IDC_SAVE + IDC_FAIL;
            }
            alert(prompt);
            window.focus();
    }});
}

function SaveAlarmVL(){
      var losschk = $("#losschk").is(":checked")?1:0;
      var timestrategy = $("#vl_time_protection").selectTime('getData');
      var str = "vlcfg -act set -timestrategy " + timestrategy;
      str += " -enable " + losschk;
        
      GetJCP({cmd:str , ParseJCP: function(result){
            var prompt = IDC_SAVE + IDC_SUCCESS;
            if(result == "Error")
            {
                prompt = IDC_SAVE + IDC_FAIL;
            }
            alert(prompt);
            window.focus();
     }});
}

//视频遮挡告警等级
var vmtype,vmtypeVal=0; 
function initVmtype(){
    try
    {
        vmtype = $("#vmtype").slider({
            slide: function (event, ui)
            {
                $("#tdVmtype").html(ui.value);
            },
            stop: function (event, ui)
            {
                vmtypeVal = ui.value;
            },
            min:1,
            max:100
        });

        $("#vmtypeVal").bind('slide', function(event, ui){
            $("#tdVmtype").html(ui.value);  
        });
    }catch(e){}
}

function ParseVMCfg(jcpobj){
    try{
        vmtypeVal = parseInt(jcpobj.thresh);
        vmtype.slider("value", vmtypeVal);
        var flag = (parseInt(jcpobj.enable)==1)?true:false;
        $("#tdVmtype").html(jcpobj.thresh);
        $("#vmchk").prop("checked",flag);
        $("#vm_time_protection").selectTime('setData',jcpobj.timestrategy)
    }catch(e){}
}

function SaveAlarmVM(){
      var vmchk = $("#vmchk").is(":checked")?1:0;
      var timestrategy = $("#vm_time_protection").selectTime('getData');
      var str = "vmaskalarmcfg -act set -timestrategy " + timestrategy;
      str += " -enable " + vmchk + " -thresh " + vmtypeVal;
        
      GetJCP({cmd:str , ParseJCP: function(result){
            var prompt = IDC_MSGBOX_SAVEOK;
            if(result == "Error")
            {
                prompt = IDC_MSGBOX_SAVEFAIL;
            }
            alert(prompt);
            window.focus();
     }});
}

function changePlace() {
    var place = parseInt($("#place").val());
    switch(place) {
        case 0:  // 晚上
            $("#td_custom_time_key").hide();
            $("#td_custom_time_value").hide();
            $("#alarmtime_day").hide();
            $("#alarmtime_night").show();
        break;
        case 1:  // 白天
            $("#td_custom_time_key").hide();
            $("#td_custom_time_value").hide();
            $("#alarmtime_night").hide();
            $("#alarmtime_day").show();
        break;
        case 3:  // 自定义
            $("#td_custom_time_key").show();
            $("#td_custom_time_value").show();
            $("#alarmtime_night").hide();
            $("#alarmtime_day").hide();
        break;
        default:
            $("#td_custom_time_key").hide();
            $("#td_custom_time_value").hide();
            $("#alarmtime_night").hide();
            $("#alarmtime_day").hide();
        break;
    }
}
function changeVoicePlace() {
    var place = parseInt($("#voice_place").val());
    switch(place) {
        case 0:  // 晚上
        $("#td_voice_custom_time_key").hide();
        $("#td_voice_custom_time_value").hide();
        $("#voice_alarmtime_day").hide();
        $("#voice_alarmtime_night").show();
        break;
        case 1:  // 白天
        $("#td_voice_custom_time_key").hide();
        $("#td_voice_custom_time_value").hide();
        $("#voice_alarmtime_night").hide();
        $("#voice_alarmtime_day").show();
        break;
        case 3:  // 自定义
        $("#td_voice_custom_time_key").show();
        $("#td_voice_custom_time_value").show();
        $("#voice_alarmtime_night").hide();
        $("#voice_alarmtime_day").hide();
        break;
        default:
          $("#td_voice_custom_time_key").hide();
          $("#td_voice_custom_time_value").hide();
          $("#voice_alarmtime_night").hide();
          $("#voice_alarmtime_day").hide();
        break;
    }
}
function changeIOPlace() {
    var place = parseInt($("#io_place").val());
    switch(place) {
    case 0:  // 晚上
        $("#td_io_custom_time_key").hide();
        $("#td_io_custom_time_value").hide();
        $("#io_alarmtime_day").hide();
        $("#io_alarmtime_night").hide();
        break;
    case 1:  // 白天
        $("#td_io_custom_time_key").hide();
        $("#td_io_custom_time_value").hide();
        $("#io_alarmtime_night").hide();
        $("#io_alarmtime_day").hide();
        break;
    case 3:  // 自定义
        $("#td_io_custom_time_key").show();
        $("#td_io_custom_time_value").show();
        $("#io_alarmtime_day").hide();
        $("#io_alarmtime_night").hide();
        break;
    default: // 全天
        $("#td_io_custom_time_key").hide();
        $("#td_io_custom_time_value").hide();
        $("#io_alarmtime_day").hide();
        $("#io_alarmtime_night").hide();
        break;
    }
}

function initVoiceLightLink() {
    if(initVideoShow>0){
        initVideoShow=0;
        document.IPCamera.IPCStopPreview(0);
        $("#motion_ipcamer").html('');
    }
    g_curr_tab = "IDC_VOICE_LIGHT_LINK";

    GetJCP({cmd: "lightalarmcfg -act list", ParseJCP: function(jcpobj){
        $("input[name='LIGHT_ALARM_SWITCH'][value="+parseInt(jcpobj.enable)+"]").attr("checked",true);
        $("#place").val(jcpobj.place);
        $("#beginhour").val(jcpobj.beginhour);
        $("#beginmin").val(jcpobj.beginmin);
        $("#endhour").val(jcpobj.endhour);
        $("#endmin").val(jcpobj.endmin);

        switch(parseInt(jcpobj.place)) {
            case 0:  // 晚上
            $("#td_custom_time_key").hide();
            $("#td_custom_time_value").hide();
            $("#alarmtime_day").hide();
            $("#alarmtime_night").show();
            break;
            case 1:  // 白天
            $("#td_custom_time_key").hide();
            $("#td_custom_time_value").hide();
            $("#alarmtime_night").hide();
            $("#alarmtime_day").show();
            break;
            case 3:  // 自定义
            $("#td_custom_time_key").show();
            $("#td_custom_time_value").show();
            $("#alarmtime_night").hide();
            $("#alarmtime_day").hide();
            break;
            default:
            $("#td_custom_time_key").hide();
            $("#td_custom_time_value").hide();
            $("#alarmtime_night").hide();
            $("#alarmtime_day").hide();
            break;
        }

        $("#custom_time").val(jcpobj.time);
    }});
    
    GetJCP({cmd: "audioalarmcfg -act list", ParseJCP: function(jcpobj){
        if (jcpobj.show == 0) {
            $("#td_voice_alarm").hide();
        } else {
            $("#td_voice_alarm").show();
        }
        $("input[name='VOICE_ALARM_SWITCH'][value="+parseInt(jcpobj.enable)+"]").attr("checked",true);
        $("input[name='ALARM_VOICE'][value="+parseInt(jcpobj.type)+"]").attr("checked",true);
        
        $("#voice_place").val(jcpobj.place);
        $("#voice_beginhour").val(jcpobj.beginhour);
        $("#voice_beginmin").val(jcpobj.beginmin);
        $("#voice_endhour").val(jcpobj.endhour);
        $("#voice_endmin").val(jcpobj.endmin);

        switch(parseInt(jcpobj.place)) {
            case 0:  // 晚上
            $("#td_voice_custom_time_key").hide();
            $("#td_voice_custom_time_value").hide();
            $("#voice_alarmtime_day").hide();
            $("#voice_alarmtime_night").show();
            break;
            case 1:  // 白天
            $("#td_voice_custom_time_key").hide();
            $("#td_voice_custom_time_value").hide();
            $("#voice_alarmtime_night").hide();
            $("#voice_alarmtime_day").show();
            break;
            case 3:  // 自定义
            $("#td_voice_custom_time_key").show();
            $("#td_voice_custom_time_value").show();
            $("#voice_alarmtime_night").hide();
            $("#voice_alarmtime_day").hide();
            break;
            default:
            $("#td_voice_custom_time_key").hide();
            $("#td_voice_custom_time_value").hide();
            $("#voice_alarmtime_night").hide();
            $("#voice_alarmtime_day").hide();
            break;
        }

        $("#voice_times").val(jcpobj.times);
    }});
}

function changeCustomTime(){
    var v = $("#custom_time").val();
    var v = $("#"+oid).val();
    if(v<10)v=10;
    if(v>60)v=60;
   $("#custom_time").val(v);
}

function SaveVoiceLightLink(){

    var _jcp = "lightalarmcfg -act set";
    var time = $("#custom_time").val();

    var enable = parseInt($("input[name='LIGHT_ALARM_SWITCH']:checked").val());
    _jcp += " -enable " + enable;
    _jcp += " -place " + $("#place").val();
    _jcp += " -beginhour " + $("#beginhour").val();
    _jcp += " -beginmin " + $("#beginmin").val();
    _jcp += " -endhour " + $("#endhour").val();
    _jcp += " -endmin " + $("#endmin").val();
    _jcp += " -time " + $("#custom_time").val();

    GetJCPList({cmd: _jcp}); 
    if ("" == time || 10 > time || 60 < time) {
        alert(IDC_LIGHT_ALARM_LENGTH + "(10~60)" + IDC_GEN_UNIT_SECOND);
        window.focus();
    } else {
        alert(IDC_SAVE + IDC_SUCCESS);
        window.focus();
    }
}

function SaveVoiceAlarm() {
    var _jcp = "audioalarmcfg -act set";

    var enable = parseInt($("input[name='VOICE_ALARM_SWITCH']:checked").val());
    var type = parseInt($("input[name='ALARM_VOICE']:checked").val());
    _jcp += " -enable " + enable;
    _jcp += " -type " + type;
    _jcp += " -place " + $("#voice_place").val();
    _jcp += " -beginhour " + $("#voice_beginhour").val();
    _jcp += " -beginmin " + $("#voice_beginmin").val();
    _jcp += " -endhour " + $("#voice_endhour").val();
    _jcp += " -endmin " + $("#voice_endmin").val();
    _jcp += " -times " + $("#voice_times").val();

    GetJCPList({cmd: _jcp}); 
    alert(IDC_SAVE + IDC_SUCCESS);
    window.focus();
}

function initIOAlarm() {
    if(initVideoShow>0){
        initVideoShow=0;
        document.IPCamera.IPCStopPreview(0);
        $("#motion_ipcamer").html('');
    }
    g_curr_tab = "IDC_IO_ALARM";

     GetJCP({cmd: "ioalarmcfg -act list", ParseJCP: function(jcpobj){
        $("input[name='IO_ALARM_SWITCH'][value="+parseInt(jcpobj.enable)+"]").attr("checked",true);
        
        $("#io_place").val(jcpobj.place);
        $("#io_beginhour").val(jcpobj.beginhour);
        $("#io_beginmin").val(jcpobj.beginmin);
        $("#io_endhour").val(jcpobj.endhour);
        $("#io_endmin").val(jcpobj.endmin);

        switch(parseInt(jcpobj.place)) {             
        case 0:   // 晚上
            $("#td_io_custom_time_key").hide();
            $("#td_io_custom_time_value").hide();
            $("#io_alarmtime_day").hide();        
            $("#io_alarmtime_night").show();      
            break;                                   
        case 1:   // 白天
            $("#td_io_custom_time_key").hide();
            $("#td_io_custom_time_value").hide();
            $("#io_alarmtime_night").hide();      
            $("#io_alarmtime_day").show();
            break;
        case 3:   // 自定义 
            $("#td_io_custom_time_key").show();
            $("#td_io_custom_time_value").show();
            $("#io_alarmtime_night").hide();      
            $("#io_alarmtime_day").hide();
            break;
        default:  // 全天
            $("#td_io_custom_time_key").hide();
            $("#td_io_custom_time_value").hide();
            $("#io_alarmtime_night").hide();      
            $("#io_alarmtime_day").hide();
            break;
        }
    }});
}

function SaveIOAlarm(){

    var _jcp = "ioalarmcfg -act set";

    var enable = parseInt($("input[name='IO_ALARM_SWITCH']:checked").val());
    _jcp += " -enable " + enable;
    _jcp += " -place " + $("#io_place").val();
    _jcp += " -beginhour " + $("#io_beginhour").val();
    _jcp += " -beginmin " + $("#io_beginmin").val();
    _jcp += " -endhour " + $("#io_endhour").val();
    _jcp += " -endmin " + $("#io_endmin").val();
   
    GetJCPList({cmd: _jcp}); 
    alert(IDC_SAVE + IDC_SUCCESS);
    window.focus();
}


function initAlarmLink(){
    if(initVideoShow>0){
        initVideoShow=0;
        document.IPCamera.IPCStopPreview(0);
        $("#motion_ipcamer").html('');
    }
    g_curr_tab = "IDC_ALARMLINKAGE";


    /*if(ptz_alarm_in > 0){

        //获取报警输入配置信息
        GetJCPList({cmd: "ca2aicfg -act list", async: false , ParseJCP: function(jcpobj){
            if(jcpobj != 'Error'){
                var alarmArr = jcpobj.split("#");
                var num= alarmArr[0].split(";")[0].split("=")[1];
                for(var i=0;i<num;i++){
                    aiLinkage[i][0] = parseInt(GetRtspKeyStr(alarmArr[i],"interval"));
                    aiLinkage[i][1] = parseInt(GetRtspKeyStr(alarmArr[i],"acen"));
                    aiLinkage[i][2] = parseInt(GetRtspKeyStr(alarmArr[i],"emailen"));
                    aiLinkage[i][3] = parseInt(GetRtspKeyStr(alarmArr[i],"ao0en"));
                    aiLinkage[i][4] = parseInt(GetRtspKeyStr(alarmArr[i],"ao1en"));
                    aiLinkage[i][5] = parseInt(GetRtspKeyStr(alarmArr[i],"recorden"));
                    aiLinkage[i][6] = parseInt(GetRtspKeyStr(alarmArr[i],"ftpen"));
                    aiLinkage[i][7] = parseInt(GetRtspKeyStr(alarmArr[i],"sounden"));
                    aiLinkage[i][8] = parseInt(GetRtspKeyStr(alarmArr[i],"captureen"));
                    aiLinkage[i][9] = parseInt(GetRtspKeyStr(alarmArr[i],"linktype"));
                    aiLinkage[i][10] = parseInt(GetRtspKeyStr(alarmArr[i],"preset"));

                    SetAiCheck();//设置报警输入报警
                }
            }
                
        }});
        


    }*/



    //FtpClient
    //GetJCP({cmd: "ftpclicfg -act list", ParseJCP: function(jcpobj){
     //   $("input[name='cbFtp'][value="+parseInt(jcpobj.type)+"]").attr("checked",true);
    //}});
     //报警声音类型
    GetJCP({cmd: "alarmaudio -act list", ParseJCP: function(jcpobj){
        $("input[name='radioAudioType'][value="+parseInt(jcpobj.alarm_type)+"]").attr("checked",true);
    }});

    //视频丢失联动
    GetJCP({cmd: "ca2vlcfg -act list", ParseJCP: function(jcpobj){
        $("#cbVLEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        //$("#cbVLAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbVLInterval").val(jcpobj.interval);
    }});
    //移动侦测联动
    /*GetJCP({cmd: "ca2mdcfg -act list", ParseJCP: function(jcpobj){
        $("#cbMDEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        $("#cbMDAudio").prop("checked",parseInt(jcpobj.sounden)==1);
        //$("#cbMDRecord").prop("checked",parseInt(jcpobj.recorden)==1);
        //$("#cbMDFtp").prop("checked",parseInt(jcpobj.ftpen)==1);
        //$("#cbMDCapture").prop("checked",parseInt(jcpobj.captureen)==1);
        //$("#cbMDAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbMDInterval").val(jcpobj.interval);
    }});*/
    //视频遮挡联动                                      
    GetJCP({cmd: "ca2vmcfg -act list", ParseJCP: function(jcpobj){
        $("#cbVMEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        //$("#cbVMRecord").prop("checked",parseInt(jcpobj.recorden)==1);
        //$("#cbVMFtp").prop("checked",parseInt(jcpobj.ftpup)==1);
        //$("#cbVMCapture").prop("checked",parseInt(jcpobj.captureen)==1);
        //$("#cbVMAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbVMInterval").val(jcpobj.interval);
    }});
    //网线断开联动
    /*GetJCP({cmd: "ca2linkbroken -act list", ParseJCP: function(jcpobj){
        $("#cbNETRecord").prop("checked",parseInt(jcpobj.recorden)==1);
        $("#cbNETCapture").prop("checked",parseInt(jcpobj.captureen)==1);
        $("#cbNETAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbNETInterval").val(jcpobj.interval);
    }});*/
    //磁盘错误
    /*GetJCP({cmd: "ca2diskerror -act list", ParseJCP: function(jcpobj){
        $("#cbDEEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        //$("#cbDEAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbDEInterval").val(jcpobj.interval);
    }});*/
    //IP冲突联动
    /*GetJCP({cmd: "ca2ipconflict -act list", ParseJCP: function(jcpobj){
        $("#cbIPRecord").prop("checked",parseInt(jcpobj.recorden)==1);
        $("#cbIPCapture").prop("checked",parseInt(jcpobj.captureen)==1);
        $("#cbIPAO").prop("checked",parseInt(jcpobj.ao0en)==1);
        $("#cbIPInterval").val(jcpobj.interval);
    }});*/
    //越界侦测报警联动
    GetJCP({cmd: "ca2vglinecfg -act list", ParseJCP: function(jcpobj){
        $("#cbVGLineAudio").prop("checked",parseInt(jcpobj.sounden)==1);
        $("#cbVGLineEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        $("#cbVGLineInterval").val(jcpobj.interval);
    }});
    //区域侦测报警联动
    GetJCP({cmd: "ca2vgrectcfg -act list", ParseJCP: function(jcpobj){
        $("#cbVGRectAudio").prop("checked",parseInt(jcpobj.sounden)==1);
        $("#cbVGRectEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        $("#cbVGRectInterval").val(jcpobj.interval);
    }});
    //人形侦测报警联动
    GetJCP({cmd: "ca2humandetectcfg -act list", ParseJCP: function(jcpobj){
        $("#cbHumanAudio").prop("checked",parseInt(jcpobj.sounden)==1);
        $("#cbHumanEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        $("#cbHumanInterval").val(jcpobj.interval);
    }});
    //人车侦测报警联动
    GetJCP({cmd: "ca2cardetectcfg -act list", ParseJCP: function(jcpobj){
        $("#cbCarAudio").prop("checked",parseInt(jcpobj.sounden)==1);
        $("#cbCarEmail").prop("checked",parseInt(jcpobj.emailen)==1);
        $("#cbCarInterval").val(jcpobj.interval);
    }});
}

var _prseset_html = '<option value="0"></option>';
var _no_preset_html = '<option value="0"></option>';

/*======================================================================================
    当用户点击报警输入checkbox框的时候，保存参数到aiLinkage数组中
    arguments[0]:           ;this 指针
    arguments[1]:           ;this 指针指向对象在数组中的下标值
======================================================================================*/


function AutoSaveAiChk(obj, index)
{
    var selindex = parseInt($("#aisel").val());
    var temp = 0;
    var tempStr;
    if (obj == null)  return false;
    
    if (0 > index || index > 10)  return false;
   
    if(0 == index){
        var  v = $("#aiInterval").val();
        if(v<3) v=3;
        if(v>60) v=60;
        
        $("#aiInterval").val(v);
        aiLinkage[selindex][index] = v;
        aiLinkageChg[selindex] = 1;
    }else if (index == 9 || index == 10) {
        aiLinkageChg[selindex] = 1;
        aiLinkage[selindex][index] = $(obj).val();

        if(index == 9){
            if($(obj).val() == 0){
                $("#ptzParam").html('').attr("disabled",true);
            }else if($(obj).val() == 1){
                $("#ptzParam").html(_prseset_html).attr("disabled",false);
            }else{
                $("#ptzParam").html(_no_preset_html).attr("disabled",false);
            }
        }
    }else{
        if (obj.type.toLowerCase() == "checkbox")
        {
            //设置参数值是否变动的标志
            if (obj.checked != aiLinkage[selindex][index])
            {
                aiLinkageChg[selindex] = 1;
            }
            if (obj.checked == false)
            {
                aiLinkage[selindex][index] = 0;
            }
            else if (obj.checked == true)
            {
                aiLinkage[selindex][index] = 1;
            }
        }
        else if (obj.type.toLowerCase() == "text")
        {
            //设置参数值是否变动的标志
            if (obj.value != aiLinkage[selindex][index])
            {
                aiLinkageChg[selindex] = 1;
            }
            aiLinkage[selindex][index] = obj.value;
        }
    }
    window.focus();
}

/*====================================================================
    设置报警输入参数
====================================================================*/
function SetAiCheck()
{
    var selIndex = parseInt($("#aisel").val());
     
    var objId = new Array("chkAIAC", "chkAIEmail", "chkAIAO0", "chkAIAO1", "chkAIRcrd", "chkAIFtp", "chkAISound", "chkAICapture");
    for (var i = 0; i < objId.length; i ++)
    {
        $("#" + objId[i]).prop("checked", aiLinkage[selIndex][i + 1] == 1);
    }

    if(aiLinkage[selIndex][9] == 0){
        $("#ptzParam").html('').attr("disabled",true);
    }else if(aiLinkage[selIndex][9] == 1){
        $("#ptzParam").html(_prseset_html).attr("disabled",false);
    }else{
        $("#ptzParam").html(_no_preset_html).attr("disabled",false);
    }
    $("#ptzAction").val(aiLinkage[selIndex][9]);
    $("#ptzParam").val(aiLinkage[selIndex][10]);
    $("#aiInterval").val(aiLinkage[selIndex][0]);
    window.focus();
}

function changeInterval(oid){
    var v = $("#"+oid).val();
    if(v<3)v=3;
    if(v>60)v=60;
    $("#"+oid).val(v);
}

function mdFtpCheck(){
    if($("#cbMDFtp").is(":checked") && $("input[name='cbFtp']:checked").val()==1){
        $("#cbMDRecord").prop("checked","checked");
    }
}

function vmFtpCheck(){
    if($("#cbVMFtp").is(":checked") && $("input[name='cbFtp']:checked").val()==1){
        $("#cbVMRecord").prop("checked","checked");
    }
}

function SaveAlarmLink(){
    //if($("#cbMDFtp").is(":checked") && $("input[name='cbFtp']:checked").val()==1){
     //   $("#cbMDRecord").prop("checked","checked");
    //}

    //if($("#cbVMFtp").is(":checked") && $("input[name='cbFtp']:checked").val()==1){
    //    $("#cbVMRecord").prop("checked","checked");
    //}

    var radioAudioType = parseInt($("input[name='radioAudioType']:checked").val());

    var VLEmail = $("#cbVLEmail").is(":checked")?1:0;
    //var VLAO = $("#cbVLAO").is(":checked")?1:0;
    var VLInterval = $("#cbVLInterval").val();


    var DEEmail = $("#cbDEEmail").is(":checked")?1:0;
    //var DEAO = $("#cbDEAO").is(":checked")?1:0;
    var DEInterval = $("#cbDEInterval").val();

    var VMEmail = $("#cbVMEmail").is(":checked")?1:0;
    //var VMRecord = $("#cbVMRecord").is(":checked")?1:0;
    //var VMFtp = $("#cbVMFtp").is(":checked")?1:0;
    //var VMCapture = $("#cbVMCapture").is(":checked")?1:0;
    //var VMAO = $("#cbVMAO").is(":checked")?1:0;
    var VMInterval = $("#cbVMInterval").val();
    

    var MDEmail = $("#cbMDEmail").is(":checked")?1:0;
    var MDAudio = $("#cbMDAudio").is(":checked")?1:0;
    //var MDRecord = $("#cbMDRecord").is(":checked")?1:0;
    //var MDFtp = $("#cbMDFtp").is(":checked")?1:0;
    //var MDCapture = $("#cbMDCapture").is(":checked")?1:0;
    //var MDAO = $("#cbMDAO").is(":checked")?1:0;
    var MDInterval = $("#cbMDInterval").val();
    
    //var NETRecord = $("#cbNETRecord").is(":checked")?1:0;
    //var NETCapture = $("#cbNETCapture").is(":checked")?1:0;
    //var NETAO = $("#cbNETAO").is(":checked")?1:0;
    //var NETInterval = $("#cbNETInterval").val();

    //var IPRecord = $("#cbIPRecord").is(":checked")?1:0;
    //var IPCapture = $("#cbIPCapture").is(":checked")?1:0;
    //var IPAO = $("#cbIPAO").is(":checked")?1:0;
    //var IPInterval = $("#cbIPInterval").val();
    
    var jcpstr = "alarmaudio -act set -alarm_type " + radioAudioType;
    GetJCP({cmd: jcpstr});

    var VGLineEmail = $("#cbVGLineEmail").is(":checked")?1:0;
    var VGLineAudio = $("#cbVGLineAudio").is(":checked")?1:0;
    var VGLineInterval = $("#cbVGLineInterval").val();
    var VGRectEmail = $("#cbVGRectEmail").is(":checked")?1:0;
    var VGRectAudio = $("#cbVGRectAudio").is(":checked")?1:0;
    var VGRectInterval = $("#cbVGRectInterval").val();
    var VGHumanEmail = $("#cbHumanEmail").is(":checked")?1:0;
    var VGHumanAudio = $("#cbHumanAudio").is(":checked")?1:0;
    var VGHumanInterval = $("#cbHumanInterval").val();
    //人车
    var VGCarEmail = $("#cbCarEmail").is(":checked")?1:0;
    var VGCarAudio = $("#cbCarAudio").is(":checked")?1:0;
    var VGCarInterval = $("#cbCarInterval").val();
    
    var jcpstr = "ca2vlcfg -act set -interval "+parseInt(VLInterval)+" -emailen "+VLEmail;
    GetJCP({cmd: jcpstr});


    //var jcpstr = "ca2diskerror -act set -interval "+parseInt(DEInterval)+" -emailen "+DEEmail;
    //GetJCP({cmd: jcpstr});

    var jcpstr = "ca2vmcfg -act set -interval "+parseInt(VMInterval)+" -emailen "+VMEmail;
    GetJCP({cmd: jcpstr});

    var jcpstr = "ca2mdcfg -act set -interval "+parseInt(MDInterval)+" -emailen "+MDEmail +" -sounden "+ MDAudio;
    GetJCP({cmd: jcpstr});

    var jcpstr = "ca2vglinecfg -act set -interval "+parseInt(VGLineInterval)+" -emailen "+VGLineEmail+" -sounden "+VGLineAudio;
    GetJCP({cmd: jcpstr});

    var jcpstr = "ca2vgrectcfg -act set -interval "+parseInt(VGRectInterval)+" -emailen "+VGRectEmail+" -sounden "+VGRectAudio;
    GetJCP({cmd: jcpstr});
    
    var jcpstr = "ca2humandetectcfg -act set -interval "+parseInt(VGHumanInterval)+" -emailen "+VGHumanEmail+" -sounden "+VGHumanAudio;
    GetJCP({cmd: jcpstr});

    //车辆侦测联动
    var jcpstr = "ca2cardetectcfg -act set -interval "+parseInt(VGCarInterval)+" -emailen "+VGCarEmail+" -sounden "+VGCarAudio;
    GetJCP({cmd: jcpstr});
    //同时项FtpClient设置发送命令
    //jcpstr = "ftpclicfg -act set  -type " + cbFtp;
    //GetJCP({cmd: jcpstr});
    //var jcpstr = "ca2linkbroken -act set -interval "+parseInt(NETInterval)+" -captureen "+NETCapture+" -recorden "+NETRecord + " -ao0en "+NETAO;
    //GetJCP({cmd: jcpstr});
   // var jcpstr = "ca2ipconflict -act set -interval "+parseInt(IPInterval)+" -captureen "+IPCapture+" -recorden "+IPRecord + " -ao0en "+IPAO;
    //GetJCP({cmd: jcpstr});
   /* if(ptz_alarm_in >0){
        for (var i = 0; i < ptz_alarm_in; i++)
        {
            if (aiLinkageChg[i] == 1) //判断各通道数据是否改变
            {
               jcpstr = "ca2aicfg -act set -aisel " + i + " -interval " + aiLinkage[i][0] + " -acen " + aiLinkage[i][1] + " -emailen " + aiLinkage[i][2]
                           + " -ao0en " + aiLinkage[i][3] + " -ao1en " + aiLinkage[i][4] + " -recorden " + aiLinkage[i][5] + " -ftpen " + aiLinkage[i][6]
                           + " -sounden " + aiLinkage[i][7] + " -captureen " + aiLinkage[i][8] + " -linktype " + aiLinkage[i][9] + " -preset " + aiLinkage[i][10];
               GetJCP({cmd: jcpstr});
            }
        }
    }*/

    alert(IDC_SAVE + IDC_SUCCESS);
    window.focus();
}

function SimulateAlarm(){
    var alarmType = parseInt($("#alarmType").val());
    var enSimAL = $("#enSimAL").is(":checked")?1:0;
    var jcpstr;
    if(alarmType >= 80){
        jcpstr = "alarmtest -act set -alarmtype 8 -chn "+(alarmType-80)+" -filter " + enSimAL;
    }else{
        jcpstr = "alarmtest -act set -alarmtype " + alarmType + " -filter " + enSimAL;
    }
    GetJCP({cmd: jcpstr,ParseJCP: function(result){
        if(result == "Error"){
            $("#simstatus").html(IDC_SIMULATE_FAILURE);
        }else{
            $("#simstatus").html(IDC_SIMULATE_SUCCESS);
        }
         $(document).oneTime(1000, function (result){
            $("#simstatus").empty();
        });
    }});
    window.focus();
}



$(window).resize(function(){
    var $w = $(window).width();
    if($w < 1150){
       $("body").css("width",1150);
    }else{
       $("body").css("width",'99%');
    }
});
