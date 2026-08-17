var  ptz_alarm_in =  $.cookie("has_alarmin");
$(function(){
    initAlarmTime();
    initVoiceLightLink();
    initInputChannel();
})

function zeroAdd(num){
	if(num < 10){
		return "0" + num;
	}
	return num;
}

function initAlarmTime(){
	var _html_sound_time1 = '';
	var _html_sound_time2 = '';
	var _html_sound_time3 = '';
	var _html_sound_time4 = '';
	var _html_light_time1 = '';
	var _html_light_time2 = '';
	var _html_light_time3 = '';
	var _html_light_time4 = '';
	_html_sound_time1 += '<select id="beginhour" style="width:50px;margin-left:0px;" class="sysinput2">';
	_html_sound_time2 += '<select id="beginmin"  style="width:50px;margin-left:0px;" class="sysinput2">';
	_html_sound_time3 += '<select id="endhour"   style="width:50px;margin-left:0px;" class="sysinput2">';
	_html_sound_time4 += '<select id="endmin"    style="width:50px;margin-left:0px;" class="sysinput2">';
	_html_light_time1 += '<select id="voice_beginhour" style="width:50px;margin-left:0px;" class="sysinput2">';
	_html_light_time2 += '<select id="voice_beginmin"  style="width:50px;margin-left:0px;" class="sysinput2">';
	_html_light_time3 += '<select id="voice_endhour"   style="width:50px;margin-left:0px;" class="sysinput2">';
	_html_light_time4 += '<select id="voice_endmin"    style="width:50px;margin-left:0px;" class="sysinput2">';
	for(var i=0;i<24;i++){
		 _html_sound_time1 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
		 _html_sound_time3 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
		 _html_light_time1 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
		 _html_light_time3 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
	}
	
	for(var i=0;i<60;i++){
		 _html_sound_time2 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
		 _html_sound_time4 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
		 _html_light_time2 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
		 _html_light_time4 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
	}
	
	_html_sound_time1 += '</select>';
	_html_sound_time2 += '</select>';
	_html_sound_time3 += '</select>';
	_html_sound_time4 += '</select>';
	
	_html_light_time1 += '</select>';
	_html_light_time2 += '</select>';
	_html_light_time3 += '</select>';
	_html_light_time4 += '</select>';
	
	$("#td_sound_custom_time_value").html(_html_sound_time1 + ":" + _html_sound_time2 + "&nbsp;&nbsp;~&nbsp;&nbsp;" + _html_sound_time3 + ":" + _html_sound_time4);
	
	$("#td_voice_custom_time_value").html(_html_light_time1 + ":" + _html_light_time2 + "&nbsp;&nbsp;~&nbsp;&nbsp;" + _html_light_time3 + ":" + _html_light_time4);
}

function changePlace() {
	var place = $("#place").val();
	if (place == 3)
	{
		$("#td_sound_custom_time_key").show();
		$("#td_sound_custom_time_value").show();
	} else {
		$("#td_sound_custom_time_key").hide();
		$("#td_sound_custom_time_value").hide();
	}
}
function changeVoicePlace() {
	var place = $("#voice_place").val();
	if (place == 3)
	{
		$("#td_voice_custom_time_key").show();
		$("#td_voice_custom_time_value").show();
	} else {
		$("#td_voice_custom_time_key").hide();
		$("#td_voice_custom_time_value").hide();
	}
}
function changeIOPlace() {
	var place = $("#io_place").val();
	if (place == 3)
	{
		$("#td_io_custom_time_key").show();
		$("#td_io_custom_time_value").show();
	} else {
		$("#td_io_custom_time_key").hide();
		$("#td_io_custom_time_value").hide();
	}
}

function initVoiceLightLink() {
    g_curr_tab = "IDC_VOICE_LIGHT_LINK";

    GetJCP({cmd: "lightalarmcfg -act list", ParseJCP: function(jcpobj){
        $("input[name='LIGHT_ALARM_SWITCH'][value="+parseInt(jcpobj.enable)+"]").attr("checked",true);
        $("#place").val(jcpobj.place);
        $("#beginhour").val(jcpobj.beginhour);
        $("#beginmin").val(jcpobj.beginmin);
        $("#endhour").val(jcpobj.endhour);
        $("#endmin").val(jcpobj.endmin);

		if (jcpobj.place == 3)
		{
			$("#td_sound_custom_time_key").show();
			$("#td_sound_custom_time_value").show();
		} else {
			$("#td_sound_custom_time_key").hide();
			$("#td_sound_custom_time_value").hide();
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

		if (jcpobj.place == 3)
		{
			$("#td_voice_custom_time_key").show();
			$("#td_voice_custom_time_value").show();
		} else {
			$("#td_voice_custom_time_key").hide();
			$("#td_voice_custom_time_value").hide();
		}
        $("#voice_times").val(jcpobj.times);
    }});
}

function changeCustomTime(){
	var v = $("#custom_time").val();
    if(v<10)v=10;
    if(v>60)v=60;
   $("#custom_time").val(v);
}

function SaveVoiceLightLink(){

	var _jcp = "lightalarmcfg -act set";

    var enable = parseInt($("input[name='LIGHT_ALARM_SWITCH']:checked").val());
    var custom_time = $("#custom_time").val();

    _jcp += " -enable " + enable;
    _jcp += " -place " + $("#place").val();
    _jcp += " -beginhour " + $("#beginhour").val();
    _jcp += " -beginmin " + $("#beginmin").val();
    _jcp += " -endhour " + $("#endhour").val();
    _jcp += " -endmin " + $("#endmin").val();
    _jcp += " -time " + custom_time;

	GetJCPList({cmd: _jcp}); 
    parent.paramSaveTip(IDC_SAVE + IDC_SUCCESS);
    window.focus();
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
    parent.paramSaveTip(IDC_SAVE + IDC_SUCCESS);
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

//读取输入通道，初始化相关信息
function initInputChannel(){
    //模拟报警类别
    var _simulateAlarmTypeHtml = '';
    var _inputChannelHtml = '';
    var _allChannel_html = '';

    _simulateAlarmTypeHtml += IDC_ALARM_TYPE;
    _simulateAlarmTypeHtml += '<select id="alarmType" class="sysinput2 select">';
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

	 _simulateAlarmTypeHtml += '<option value="1">'+IDC_MD_LINKAGE+'</option>';
    _simulateAlarmTypeHtml += '<option value="2">'+IDC_MD_VGLINE+'</option>';
    _simulateAlarmTypeHtml += '<option value="3">'+IDC_MD_VGRECT+'</option>';
    _simulateAlarmTypeHtml += '<option value="13">'+IDC_HUMAN_DETECTION+'</option>';
    _simulateAlarmTypeHtml += '<option value="4">'+IDC_VL_LINKAGE+'</option>';
    _simulateAlarmTypeHtml += '<option value="12">'+IDC_VM_LINKAGE+'</option>';
    _simulateAlarmTypeHtml += '<option value="6">'+JALARM_TYPE_DISK_ERR+'</option>';
    _simulateAlarmTypeHtml += '<option value="7">'+IDC_ALARM_NET_DISCONNECT+'</option>';
    _simulateAlarmTypeHtml += '<option value="8">'+IDC_ALARM_IP_CONFLICT+'</option>';
    _simulateAlarmTypeHtml += '</select>&nbsp;&nbsp;&nbsp;&nbsp;';

    _simulateAlarmTypeHtml += '<label for="enSimAL">'+IDC_SIMULATE_FILTER_RULE+'</label>';
    _simulateAlarmTypeHtml += '<input type="checkbox" id="enSimAL"/>&nbsp;&nbsp;&nbsp;&nbsp;';
    _simulateAlarmTypeHtml += '<button style="width:85px;margin-left:0px;margin-top:0px;" class="btn btn-inverse btn-black index_btn"  onclick="SimulateAlarm();">'+IDC_SIMULATE+'</button>';
    _simulateAlarmTypeHtml += '&nbsp;&nbsp;&nbsp;&nbsp;<font id="simstatus" ></font>';

    $("#tdSimulate").html(_simulateAlarmTypeHtml);

    $("#spanCopy").html(_allChannel_html);

}
