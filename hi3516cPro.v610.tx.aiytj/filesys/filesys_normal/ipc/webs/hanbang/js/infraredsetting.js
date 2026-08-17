
$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        showInfraredSet();
    }
})
function zeroAdd(num){
    if(num < 10){
        return "0" + num;
    }
    return num;
}
    
var sensitivitySlider  = null;
var sensitivitySlider_open = null;
var sensitivitySlider_close = null;
var lighting_level_slider = null;
var lightObj;
var wrapFlag = false;
function showInfraredSet(){
    var _html_night_time1 = '';
	  var _html_night_time2 = '';
	  var _html_night_time3 = '';
	  var _html_night_time4 = '';
	  _html_night_time1 += '<select id="night_time1" style="width:50px;margin-left:0px;">';
	  _html_night_time2 += '<select id="night_time2" style="width:50px;margin-left:0px;">';
	  _html_night_time3 += '<select id="night_time3" style="width:50px;margin-left:0px;">';
	  _html_night_time4 += '<select id="night_time4" style="width:50px;margin-left:0px;">';
	  for(var i=0;i<24;i++){
		    _html_night_time1 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
		    _html_night_time3 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
	  }
	  for(var i=0;i<60;i++){
		    _html_night_time2 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
		    _html_night_time4 += '<option value="'+i+'">'+zeroAdd(i)+'</option>';
	  }
	  _html_night_time1 += '</select>';
	  _html_night_time2 += '</select>';
	  _html_night_time3 += '</select>';
	  _html_night_time4 += '</select>';
    $("#tdNightTime").html(_html_night_time1 + ":" + _html_night_time2 + "&nbsp;&nbsp;~&nbsp;&nbsp;" + _html_night_time3 + ":" + _html_night_time4);
  
	  GetJCP({cmd: "lightextcfg -act list ", ParseJCP: function(jcpObj){
			  $("#switchdevtype").val(jcpObj.devtype);
			  $("#switchmode").val(jcpObj.irswitchmode);
			  changeDevtypeMode();
	  }});
}

function showIrContend(){
		$(".irsetting").hide();
		var switchmode = $("#switchmode").val();
		if(switchmode == 3){
				$("#switchdevtype").show();	
				$("#switchmode").show();	
				$("#tr_time_of_night").show();	
		} else {
				$("#switchdevtype").show();	
				$("#switchmode").show();	
				$("#tr_time_of_night").hide();	
        }
}

function changeDevtypeMode(){
		showIrContend();
}

function changeSwitchMode(){
    showIrContend();
}

function SaveInfraredSet(){
    var _cmd = "lightextcfg -act set";
    var switchdevtype = $("#switchdevtype").val();
    var switchmode = $("#switchmode").val();
    var switchctrlmode = $("#switchctrlmode").val();
    var night_time1 = $("#night_time1").val();
    var night_time2 = $("#night_time2").val();
    var night_time3 = $("#night_time3").val();
    var night_time4 = $("#night_time4").val();

    _cmd = _cmd + " -devtype " + switchdevtype + " -irswitchmode " + switchmode;
    _cmd = _cmd + " -beginhour " + night_time1 + " -beginmin " + night_time2 + " -endhour " + night_time3 + " -endmin " + night_time4;
    GetJCPList({cmd: _cmd});
    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();
}
