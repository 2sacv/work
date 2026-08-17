$(function(){
        var lf = $.cookie("loginflag_"+g_hostname);
        if (null === lf)
        {
           parent.location.href = "/login.asp";
        }else{
            _init_load();
        }
    })

   function _init_load(){
      GetJCP({cmd:"version -act list",ParseJCP:function(result){
        if(result != 'Error'){
          try{
              $("#dev_name").val(result.devname);
              $("#dev_model").html(result.devtype);
              $("#dev_num").html(result.devid);
              $("#dev_bb").html(result.kernelver);
              $("#web_bb").html(result.webver); 
              $("#server_bb").html(result.serverver);
              var cookie = GetCookieByKey("ocxversion");
              if(cookie < "6.0.0.0"){ parent.paramFailTip(IDC_OCX_VERSIONFAIL); }
              $("#osd").html(cookie);
          }catch(e){}
        }
      }});
   }


   function SaveDevName(){
    var name = $.trim($("#dev_name").val());

    var myRegExp = /["'<>%;)(&+“”\[\]!！？?\\#￥$^；【】。]/;
    if(myRegExp.test(name)){
          parent.paramFailTip(IDC_DEVNAME+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    var cArr = name.match(/[^\x00-\xff]/ig);   
    var len =  name.length + (cArr == null ? 0 : cArr.length*2); 
    if(len==0){
        parent.paramFailTip(IDC_DEVNAME_NULL);
        window.focus();
        return;
    }
    if(len>63){
        parent.paramFailTip(IDC_DEVNAME_TOO_LONG);
        window.focus();
        return;
    }

    GetJCP({cmd:"version -act set -devname \"" + name + "\"",ParseJCP:function(result){
        if(result == 'Error'){
            parent.paramFailTip(IDC_MSGBOX_SAVEFAIL);
        }else{
            parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
        }
    }});
    window.focus();
}