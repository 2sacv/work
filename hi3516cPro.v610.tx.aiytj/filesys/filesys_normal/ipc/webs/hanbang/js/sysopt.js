$(function(){
      var lf = $.cookie("loginflag_"+g_hostname);
      if (null === lf)
      {
         parent.location.href = "/login.asp";
      }else{
          _init_progress();
          _init_load();
      }
  })

function _init_load(){
    GetJCPList({cmd:"sysctrl -act list",ParseJCP: initRebootInfoAndStatus});
 }

var  progressbarSlider;
 _init_progress = function(){
    progressbarSlider = new SliderModules({
      targetId: "progressbar",
      min: 0,
      max: 100
    }); 
    progressbarSlider.create();
    progressbarSlider.onchange = function () {
        $('#.progress-label').text(progressbarSlider.getValue()+"%");
    };
 }

  //初始化自动重启信息和系统状态
 function  initRebootInfoAndStatus(result){
  try{
      var strArr = result.split("#");
      if(parseInt(GetRtspKeyStr(strArr[0],"arben"))==1){
          $("#ckarb").prop("checked",true);
      }
      $("#time").val(parseInt(GetRtspKeyStr(strArr[0],"arbtm")));
      $("#weekend").val(parseInt(GetRtspKeyStr(strArr[0],"arbweek")));
  }catch(e){}
}

function SaveAutoreboot(){
    var arbtime = $("#time").val();
    var arbweek = $("#weekend").val();
    var jcpstr = "sysctrl -act set -arben " + ($("#ckarb").is(":checked") ? 1 : 0) + " -arbtm " + arbtime + " -arbweek " + arbweek;
    GetJCP({cmd: jcpstr});
    
    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();
    return;
}

var restart_num = '0';
SysRet = function(num)
{
    restart_num = num;
    var a = num == 2 ? (IDC_MSGBOX_CONFIRM+IDC_LOADDEFAULT) : (IDC_MSGBOX_CONFIRM+IDC_RESETDEV);
    var tip = num == 2?IDC_DEFAULT_WAIT:IDC_REBOOT_WAIT;
    if(confirm(a)){
        GetJCP({cmd: "sysctrl -act set -cmd " + num,ParseJCP: function(result){
            if(result != 'Error'){
                $("#trProgress").show();
                $("#sysrettip").show();
                $("#sysrettip").html(tip);
                $("#defaultBtn").attr("disabled",true);
                $("#resetBtn").attr("disabled",true);
                $("#autoreBtn").attr("disabled",true);
                setTimeout( _progress, 1000 );
            }
        }})
    }
    window.focus();
}

var restart_progress = 0;
_progress = function(){

    var val = progressbarSlider.getValue() || 0;
    progressbarSlider.wsetValue(val+1);
    $('.progress-label').text(progressbarSlider.getValue()+"%");
    
    if(100 == parseInt(val)){
        window.clearTimeout(restart_progress)
        if(2 == parseInt(restart_num)){
            deleteCookie("lastclickmenu");
            deleteCookie("loginflag_"+g_hostname);
            parent.location.href = 'http://192.168.1.217/login.asp'  
        }else{
            deleteCookie("lastclickmenu");
            deleteCookie("loginflag_"+g_hostname);
            parent.location.href = '../login.asp'
        }
    }
    restart_progress = setTimeout( _progress, 600 );
}