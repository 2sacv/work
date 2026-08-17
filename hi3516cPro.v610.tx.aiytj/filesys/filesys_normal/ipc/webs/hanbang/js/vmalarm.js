$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
       initVM();
    }
})

function initVM(){
    $("#vm_time_protection").selectTime();
    initVmtype();
    GetJCP({cmd: "vmaskalarmcfg -act list", ParseJCP: ParseVMCfg});
}

//视频遮挡告警等级
var vmtype,vmtypeVal=0; 
function initVmtype(){
  vmtype = new SliderModules({
    targetId: "vmtype",
    min: 1,
    max: 100
  }); 
  vmtype.create();
  vmtype.onchange = function () {
      vmtypeVal = vmtype.getValue();
      $('#tdVmtype').text(vmtype.getValue());
  };
}

function ParseVMCfg(jcpobj){
    try{
        vmtypeVal = parseInt(jcpobj.thresh);
        vmtype.wsetValue(vmtypeVal);
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
        if(result == "Error"){
          parent.paramFailTip(IDC_MSGBOX_SAVEFAIL);
        }else{
          parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
        }
        window.focus();
     }});
}
