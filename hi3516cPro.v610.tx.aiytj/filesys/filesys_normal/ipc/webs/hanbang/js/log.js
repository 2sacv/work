
var ptz_alarm = 0; //是否有报警输入,1有,其他否
$(document).ready(function(){
  ptz_alarm =  $.cookie("has_alarmin");
  var lf = $.cookie("loginflag_"+g_hostname);
  if (null === lf || typeof(lf) =='undefined' || lf === "null" || 0 > parseInt(lf)){
    location.href = "/login.asp";
  }else{
    if($.cookie("graintype") == 0){
      $("#spanPlayback").hide();
    }
  	_init_language();
  	_init_click();
    initAlarmtype();
    window.document.title = IDC_LOG;

    $(".video").css("width", $(window).width()- 275 - 2);

    setTimeShow();
    setTimeout(function(){SearchSysLog();},200);

    $(window).resize(function(){
      $(".video").css("width", $(window).width()- 275 - 2);
    });
  }
});

function setTimeShow(){
  document.getElementById('spanCurrTime').innerHTML = date();
  setTimeout(function(){setTimeShow();},1000);
}

_init_click = function(){
    var language= GetCookieByKey("languages")==0?'zh-cn':'en';
    $("#sys_log_start_time").click(function(){
        WdatePicker({readOnly:true,lang:language,dateFmt:'yyyy-MM-dd HH:mm:ss'});
    });

    $("#sys_log_end_time").click(function(){
        WdatePicker({readOnly:true,lang:language,dateFmt:'yyyy-MM-dd HH:mm:ss'});
    });
    $("#alarm_log_start_time").click(function(){
        WdatePicker({readOnly:true,lang:language,dateFmt:'yyyy-MM-dd HH:mm:ss'});
    });

    $("#alarm_log_end_time").click(function(){
        WdatePicker({readOnly:true,lang:language,dateFmt:'yyyy-MM-dd HH:mm:ss'});
    });

    $("#sys_log_start_time").val(new_date()+" 00:00:00");
    $("#sys_log_end_time").val(new_date()+" 23:59:59");
    $("#alarm_log_start_time").val(new_date()+" 00:00:00");
    $("#alarm_log_end_time").val(new_date()+" 23:59:59");

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
              Set_cookie("curr_menu_id",-1);
              location.href = "setting.asp"
              break;
            case "spanLog":
              break;
            case "spanLiveview":
              location.href = "preview.asp";
              break;
            default:
              break;
          }
      });
    
    $(document).on('mouseenter', '#tbLog tbody tr:not(:first)',function(){
        $(this).css("background","rgb(49,106,197)")
    })

    $(document).on('mouseleave', '#tbLog tbody tr:not(:first)',function(){
        $(this).css("background","rgb(182,187,194)")
    })
  	
}

function _init_language(){
  $("#laLiveview").html(IDC_PLAYVIDEO);
  $("#laLog").html(IDC_LOG);
  $("#laSetting").html(IDC_PARAMETER_SET);
  $("#laPlayback").html(IDC_PLAYBACK);
  $("#laExit").html(IDC_EXIT);

  $("#search_syslog").html(IDC_SEARCH);
  $("#search_alarmlog").html(IDC_SEARCH);
  $("#sp_sys_log").html(IDC_SYSLOG_SYSLOG);
  $("#sp_alarm_log").html(IDC_ALARM_LOG);
  $("#div_sys_log_start_time").html(IDC_SYSLOG_STIME);
  $("#div_sys_log_end_time").html(IDC_SYSLOG_ETIME);
  $("#div_alarm_log_start_time").html(IDC_SYSLOG_STIME);
  $("#div_alarm_log_end_time").html(IDC_SYSLOG_ETIME);
  $("#div_alarm_log_type").html(IDC_ALARM_TYPE.replace(/[:：]/g,''));

  
  loadTd();
}

function loadTd(){
  if(mQueryType == 0){
    $("#td1").html("&nbsp;"+IDC_SYSLOG_TIME);
    $("#td2").html(IDC_SYSLOG_MODULE);
    $("#td3").html(IDC_SYSLOG_EVENT);
  }else{
    $("#td1").html("&nbsp;"+IDC_SYSLOG_TIME);
    $("#td2").html(IDC_TYPE);
    $("#td3").html(IDC_ALARM_DESC);
  }
}

function initAlarmtype(){
    var _html = '<option value="0">'+JALARM_TYPE_BEGIN+'</option>';
    if(parseInt(ptz_alarm) > 0){
        for(var i=0;i<parseInt(ptz_alarm);i++){
            _html += '<option value="'+(80+i)+'">'+IDC_ALARM_TYPE_AI_CHN+(i+1)+'</option>';
        }
    }
    _html += '<option value="1">'+JALARM_TYPE_MD+'</option>';
    _html += '<option value="2">'+IDC_MD_VGLINE+'</option>';
    _html += '<option value="3">'+IDC_MD_VGRECT+'</option>';
    _html += '<option value="13">'+IDC_HUMAN_DETECTION+'</option>';
    _html += '<option value="4">'+JALARM_TYPE_VL+'</option>';
    _html += '<option value="12">'+JALARM_TYPE_VM+'</option>';
    _html += '<option value="6">'+JALARM_TYPE_DISK_ERR+'</option>';
    _html += '<option value="7">'+IDC_ALARM_NET_DISCONNECT+'</option>';
    _html += '<option value="8">'+JALARM_TYPE_IP_CONFLICT+'</option>';
    $("#selAlarmType").html(_html);
}

var pageNum=20;
var totalPage=0;
var totalNum=0;
var currPage=1;
var mQueryType=0;//0系统日志，1报警日志

function SearchSysLog(){
    $("#sp_alarm_log").css("color", "black");
    $("#sp_sys_log").css("color", "rgb(252,173,90)");
    mQueryType = 0;
    loadTd();
    var sTime = $("#sys_log_start_time").val();
    var eTime = $("#sys_log_end_time").val();

    if(sTime.length==0){
        messageTip(IDC_STARTTIME_SELECT);
    }

    if(eTime.length==0){
        messageTip(IDC_ENDTIME_SELECT);
    }

    if(sTime>=eTime){
        messageTip(IDC_STARTTIME_BIGGER_ENDTIME);
    }

    cleanLogData();

    $("#search_syslog").attr("disabled",true);
    $("#search_alarmlog").attr("disabled",true);
    $("#search_syslog").html("<font color='red'>"+IDC_QUERYING+"</font>");

    var itemindex = (currPage-1)*pageNum+1;
    var jcpstr = "getlog -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -itemindex  "+itemindex+" -itemnum "+pageNum;
    GetJCPList({cmd: jcpstr,ParseJCP: function(result){
        var arrRst = result.split(";");
        totalNum = parseInt(arrRst[0].split("=")[1]);

        if(totalNum>0){
            totalPage = parseInt(totalNum/pageNum)+(totalNum%pageNum==0?0:1);
      
            var itemlist = arrRst[2].split("itemlist=")[1];
            showLog(itemlist);
            if(totalPage>1){
                $("#selPage").attr('disabled',false);
                $("#selPage").empty();
                for(var j=1;j<=totalPage;j++){
                    $("#selPage").append("<option value="+j+">"+j+"</option>");
                }
                $("#pageNext").attr('disabled',false);
            }else{
                $("#selPage").append("<option value='1'>1</option>");
            }

        }else{
            $("#tbLog").append("<tr><td colspan='3' align='center' >"+IDC_SYSLOG_NODATA+"</td></tr>");
        }
        $("#search_alarmlog").attr("disabled",false);
        $("#search_syslog").attr("disabled",false);
        $("#search_syslog").html(IDC_SEARCH);
    }});
}

function cleanLogData(){
    $("#selPage").empty();
    $("#tbLog tr:not(:first)").remove();
    $("#selPage").attr('disabled',true);
    $("#pagePrev").attr('disabled',true);
    $("#pageNext").attr('disabled',true);
    currPage=1;
    totalPage=0;
    totalNum=0;
}

function changePage(){
    currPage = $("#selPage").val();
    showPageButton();
    
    if(mQueryType == 0){
      doLogParam();
    }else{
      doLogParamAlarm();
    }
}

function doLogParam(){
    var sTime = $("#sys_log_start_time").val();
    var eTime = $("#sys_log_end_time").val();

   if(sTime.length==0){
        messageTip(IDC_STARTTIME_SELECT);
    }

    if(eTime.length==0){
        messageTip(IDC_ENDTIME_SELECT);
    }

    if(sTime>=eTime){
        messageTip(IDC_STARTTIME_BIGGER_ENDTIME);
    }
    var itemindex = (currPage-1)*pageNum+1;
    var jcpstr = "getlog -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -itemindex  "+itemindex+" -itemnum "+pageNum;
    changePageData(jcpstr);
}

function changePageData(jcpstr){
    $("#tbLog tr:not(:first)").remove();
    GetJCPList({cmd: jcpstr,ParseJCP: function(result){
        var itemlist = result.split(";")[2].split("itemlist=")[1];
        showLog(itemlist);
    }});
}

function showLog(itemlist){
    var itemArr = itemlist.split("#");
    var html = "";
    for(var i=0,len=itemArr.length;i<len-1;i++){
        var subitem = itemArr[i].split("|");
        html += "<tr style='height:25px;'>"
        html += "<td align='left'>&nbsp;"+subitem[0]+"</td>"
        html += "<td align='left'>"+subitem[4]+"</td>"
        html += "<td align='left'>"+subitem[5]+"</td>"
        html += "</tr>"
    }
    $("#tbLog").append(html);
}

function pagePrev(){
    currPage--;
    showPageButton();
    setPageSel();
    if(mQueryType == 0){
      doLogParam();
    }else{
      doLogParamAlarm();
    }
}

function pageNext(){
    currPage++;
    showPageButton();
    setPageSel();
    if(mQueryType == 0){
      doLogParam();
    }else{
      doLogParamAlarm();
    }
}

function showPageButton(){
    if(currPage==1){
        $("#pagePrev").attr('disabled',true);
        $("#pageNext").attr('disabled',false);
    }else if(currPage==totalPage){
        $("#pagePrev").attr('disabled',false);
        $("#pageNext").attr('disabled',true);
    }else{
        $("#pagePrev").attr('disabled',false);
        $("#pageNext").attr('disabled',false);
    }
}

function setPageSel(){
    $("#selPage").val(currPage);
}

function SearchAlarmLog(){
    $("#sp_sys_log").css("color", "black");
    $("#sp_alarm_log").css("color", "rgb(252,173,90)");
    mQueryType = 1;
    loadTd();
    var sTime = $("#alarm_log_start_time").val();
    var eTime = $("#alarm_log_end_time").val();
    var selType = $("#selAlarmType").val();

    if(sTime.length==0){
        messageTip(IDC_STARTTIME_SELECT);
    }

    if(eTime.length==0){
        messageTip(IDC_ENDTIME_SELECT);
    }

    if(sTime>=eTime){
        messageTip(IDC_STARTTIME_BIGGER_ENDTIME);
    }

    cleanLogData();

    $("#search_syslog").attr("disabled",true);
    $("#search_alarmlog").attr("disabled",true);
    $("#search_alarmlog").html("<font color='red'>"+IDC_QUERYING+"</font>");

    var itemindex = (currPage-1)*pageNum+1;
    var jcpstr;
    if(selType >= 80){
        jcpstr = "getalarmevent -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -type 8 -alarmchn "+(selType-80)+" -itemindex  "+itemindex+" -itemnum "+pageNum;
    }else{
        jcpstr = "getalarmevent -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -type "+selType+" -itemindex  "+itemindex+" -itemnum "+pageNum;
    }

    GetJCPList({cmd: jcpstr,ParseJCP: function(result){
        var arrRst = result.split(";");
        totalNum = parseInt(arrRst[0].split("=")[1]);
        if(totalNum>0){
            totalPage = parseInt(totalNum/pageNum)+(totalNum%pageNum==0?0:1);
      
            var itemlist = arrRst[2].split("itemlist=")[1];
            showLogAlarm(itemlist);
            if(totalPage>1){
                $("#selPage").attr('disabled',false);
                $("#selPage").empty();
                for(var j=1;j<=totalPage;j++){
                    $("#selPage").append("<option value="+j+">"+j+"</option>");
                }
                $("#pageNext").attr('disabled',false);
            }else{
                $("#selPage").append("<option value='1'>1</option>");
            }

        }else{
            $("#tbLog").append("<tr><td colspan='3' align='center'>"+IDC_SYSLOG_NODATA+"</td></tr>");
        }
        
        $("#search_alarmlog").html(IDC_SEARCH);
        $("#search_syslog").attr("disabled",false);
        $("#search_alarmlog").attr("disabled",false);
		
        document.getElementById("search_alarmlog").focus();
    }});

}

function doLogParamAlarm(){
    var sTime = $("#alarm_log_start_time").val();
    var eTime = $("#alarm_log_end_time").val();
    var selType = $("#selAlarmType").val();

    if(sTime.length==0){
        messageTip(IDC_STARTTIME_SELECT);
    }

    if(eTime.length==0){
        messageTip(IDC_ENDTIME_SELECT);
    }

    if(sTime>=eTime){
        messageTip(IDC_STARTTIME_BIGGER_ENDTIME);
    }
    var itemindex = (currPage-1)*pageNum+1;
    var jcpstr;
    if(selType >= 80){
        jcpstr = "getalarmevent -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -type 8 -alarmchn "+(selType-80)+" -itemindex  "+itemindex+" -itemnum "+pageNum;
    }else{
        jcpstr = "getalarmevent -act list -starttime \""+sTime+"\" -endtime \""+eTime+"\" -type "+selType+" -itemindex  "+itemindex+" -itemnum "+pageNum;
    }
    changePageDataAlarm(jcpstr);
}

function changePageDataAlarm(jcpstr){
    $("#tbLog tr:not(:first)").remove();
    GetJCPList({cmd: jcpstr,ParseJCP: function(result){
        var itemlist = result.split(";")[2].split("itemlist=")[1];
        showLogAlarm(itemlist);
    }});
}

function showLogAlarm(itemlist){
    var itemArr = itemlist.split("#");
    var html = "";
    for(var i=0,len=itemArr.length;i<len-1;i++){
        var subitem = itemArr[i].split("|");
        html += "<tr style='height:25px;'>"
        html += "<td align='left'>&nbsp;"+subitem[0]+"</td>"
        html += "<td align='left'>"+subitem[1]+"</td>"
        html += "<td align='left'>"+subitem[2]+"</td>"
        html += "</tr>"
    }
    $("#tbLog").append(html);
}

function messageTip(msg){
  $("#paramFailTip").html(msg);
  if($("#paramFailTip").is(":hidden")){
    $("#paramFailTip").show();
    setTimeout(function(){document.getElementById("paramFailTip").style.display="none";},2000);
  }
}
