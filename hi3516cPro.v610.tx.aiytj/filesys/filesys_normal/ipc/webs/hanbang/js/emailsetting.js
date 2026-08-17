$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
       GetJCP({cmd: "emailcfg -act list", ParseJCP: ParseEmailCfg});
    }
})

function ParseEmailCfg(jcpobj){
    try{
        $("#emailserver").val(jcpobj.smtpserver);
        $("#emailuser").val(jcpobj.smtpuser);
        $("#emailpasswd").val(jcpobj.smtppasswd);
        $("#emailtoaddr").val(jcpobj.toaddr);
    }catch(e){
    }
}

function SaveEmail(){
    var email = $("#emailtoaddr").val();
    var emailserver = $("#emailserver").val();
    var user = $("#emailuser").val()
    var pwd = $("#emailpasswd").val()
    if (isBlank(email))
    {
        parent.paramFailTip(IDC_EMAIL_DEST_MSG_BLANK);//邮件参数目的
        window.focus();
        return false;
    }
    if (isBlank(emailserver))
    {
        parent.paramFailTip(IDC_EMAIL_SERVER_MSG_BLANK);//邮件参数服务器
        window.focus();
        return false;
    }

    var myRegExp = /["'<>%;)(&+“”\[\]!！？?\\#￥$^；【】。]/;
    if(myRegExp.test(emailserver)){
          parent.paramFailTip(IDC_EMAIL_SERVER+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }
    if(myRegExp.test(user)){
          parent.paramFailTip(IDC_EMAIL_USERNAME+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }
    if(myRegExp.test(pwd)){
          parent.paramFailTip(IDC_EMAIL_PASSWORD+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    if(myRegExp.test(email)){
          parent.paramFailTip(IDC_EMAIL_TOADDR+IDC_NOT_CONTAIN+"\"'<>%;)(&+“”[]!！？?\\#￥$^；【】。");
          window.focus();
          return;
    }

    if (email.getBytes() > 63)
    {
        parent.paramFailTip(IDC_EMAIL_DEST_MSG_RANGE);//Email目的地址
        window.focus();
        return false;
    }
    if (emailserver.getBytes() > 63)
    {
        parent.paramFailTip(IDC_EMAIL_SERVER_MSG_RANGE);//Email服务器
        window.focus();
        return false;
    }

    //验证EMAIL地址的合法性
    var EmailRegExp = new RegExp("^\\w+([-+.]\\w+)*@\\w+([-.]\\w+)*\\.\\w+([-.]\\w+)*$","g");
    if (EmailRegExp.test(email) == false)
    {
        parent.paramFailTip(IDC_EMAIL_CHECK_FAIL);
        window.focus();
        return false;
    }
    
    //验证邮件服务器,形如:xxxx.xyz.xxx或Ip地址
    var UrlRegExp = new RegExp("([\\w-]+\\.)+[\\w-]+(/\\[\\w- ./?%&=\\]*)?","g");
    if (UrlRegExp.test(emailserver) == false)
    {
        parent.paramFailTip(IDC_SERVER_CHECK_FAIL);
        window.focus();
        return false;
    }

    jcpstr = "emailcfg -act set -smtpserver " + emailserver + " -smtpuser " + $("#emailuser").val() + 
                " -smtppasswd " + $("#emailpasswd").val() + " -toaddr " + $("#emailtoaddr").val();
    GetJCP({cmd: jcpstr});

    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();

}