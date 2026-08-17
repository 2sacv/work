$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
       initVL();
    }
})

function initVL(){
    $("#vl_time_protection").selectTime();
    GetJCP({cmd: "vlcfg -act list", ParseJCP: ParseVLCfg});
}

function ParseVLCfg(jcpobj){
  try{
    var flag = (parseInt(jcpobj.enable)==1)?true:false;
    $("#losschk").prop("checked",flag);
    $("#vl_time_protection").selectTime('setData',jcpobj.timestrategy)
  }catch(e){}
}

function SaveAlarmVL(){
      var losschk = $("#losschk").is(":checked")?1:0;
      var timestrategy = $("#vl_time_protection").selectTime('getData');
      var str = "vlcfg -act set -timestrategy " + timestrategy;
    str += " -enable " + losschk;
      
    GetJCP({cmd:str , ParseJCP: function(result){
        if(result == "Error"){
          parent.paramFailTip(IDC_MSGBOX_SAVEFAIL);
        }else{
          parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
        }
        window.focus();
   }});
}