$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        init_call();
        $("select").bind("change",function(){
             window.focus();
        }); 
    }
})

function init_call()
{
    try
    {
        var RecPath = GetCookieByKey("RecPath");
        if (-1 != RecPath)
        {
            $("#RecPath").val(RecPath);
        }
        
        var RecPackTime = parseInt(GetCookieByKey("RecPackTime"));
        if (-1 != RecPackTime)
        {
            $("#RectimeSel").val(RecPackTime);
        }
        
        var AlarmRecTime = parseInt(GetCookieByKey("AlarmRecTime"));
        if (-1 != AlarmRecTime)
        {
            $("#AlarmRecTime").val(AlarmRecTime);
        }
        
        var AlarmRecCk = parseInt(GetCookieByKey("AlarmRecCk"));
        if (-1 != AlarmRecCk)
        {
            $("#AlarmRecCk").prop("checked", (AlarmRecCk == 1) ? true : false);
        }
        
        var PreRecTime = parseInt(GetCookieByKey("PreRecTime"));
        if (-1 != PreRecTime)
        {
            $("#PreRecTime").val(PreRecTime);
        }
        
        var PreRecCk = parseInt(GetCookieByKey("PreRecCk"));
        if (-1 != PreRecCk)
        {
            $("#PreRecCk").prop("checked", (PreRecCk == 1) ? true : false);
        }
    }
    catch(E) { }
}

function SaveLocalSetting()
{
    var RectimeSel = parseInt($("#RectimeSel").val());
    var RecPath = $("#RecPath").val();
    var AlarmRecTime = $("#AlarmRecTime").val();
    var PreRecTime =$("#PreRecTime").val();

    var AlarmRecCk = 0;
    if($("#AlarmRecCk").is(':checked'))
    {
        AlarmRecCk = 1;
    }
    
    var PreRecCk = 0;
    if($("#PreRecCk").is(':checked'))
    {
        PreRecCk = 1;
    }
     
    if(RecPath == "")
    {
        parent.paramFailTip(IDC_PATH_EMPTY);
        window.focus();
        return (false);
    }
    
    //判断路径名是否有效
    if (g_is_msie)
    {
      if(document.IPCamera != undefined){
          if (false == document.IPCamera.IPCIsPathExist(RecPath))
          {
              if (false == document.IPCamera.IPCCreatePath(RecPath))
              {
                  parent.paramFailTip(IDC_PATH_VALID);
                  window.focus();
                  return false;
              }
          }
        }
    }

    var reg = /^[a-zA-Z]:[\\][a-zA-Z_0-9\\]*$/;
    if(!reg.test(RecPath)){
        parent.paramFailTip(IDC_PATH_VALID);
        window.focus();
        return false;
    }
    
    if ("" == AlarmRecTime)
    {
        parent.paramFailTip(IDC_LINK_REC_TIME_EMPTY);
        window.focus();
        return false;
    }
    else if (0 >= AlarmRecTime || 3600 < AlarmRecTime)
    {
        parent.paramFailTip(IDC_LINK_REC_TIME_FAIL);
        window.focus();
        return false;
    }
    
    if(!isNaN(AlarmRecTime) && !isNaN(PreRecTime) )
    {
        if ("" == PreRecTime || 1 > PreRecTime || 10 < PreRecTime)
        {
            parent.paramFailTip(IDC_PRE_REC_TIME + "(1~10)" + IDC_GEN_UNIT_SECOND);
            window.focus();
            return false;
        }
        else
        {
            Set_cookie("RecPath", RecPath);
            Set_cookie("RecPackTime",RectimeSel);
            Set_cookie("AlarmRecTime",parseInt(AlarmRecTime,10));
            Set_cookie("AlarmRecCk",AlarmRecCk);
            Set_cookie("PreRecTime",parseInt(PreRecTime,10));
            Set_cookie("PreRecCk",PreRecCk);
            var AlarmRecTime = $("#AlarmRecTime").val(parseInt(AlarmRecTime,10));
            var PreRecTime =$("#PreRecTime").val(parseInt(PreRecTime,10));
            
            parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
            window.focus();
            return true;        
        }
    }
    else
    {
        self.location = "localsetting.asp";
        parent.paramFailTip(IDC_GET_REC_PROMPT);
    }
    
    
}