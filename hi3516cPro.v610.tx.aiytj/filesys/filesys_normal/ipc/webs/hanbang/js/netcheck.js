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
    $("#notaddress").val("192.168.1.211");
    $("#notcount").val("4");
    $("#notsize").val("16");
    $("#notresult").val();
    $("#frmsubmit").attr("src","netchecksub.asp");
}

var pingsetaddr ,pingsetsize , pingsetcnt;
var pingcnt = 0, pingdelay = 0;

function PingSubmit()
{
    window.clearTimeout(pingdelay);
    
    $("#frmsubmit").contents().find("#address").val(pingsetaddr);
    $("#frmsubmit").contents().find("#size").val(pingsetsize);
    pingcnt--;
    
    document.getElementById('btnPing').disabled = true;
    $("#frmsubmit").contents().find("#netcheck").submit();
}

function Ping()
{
    if (false == IPCheck())
    {
        window.focus();
        return false;
    }
    $("#notresult").val('');
    
    pingmsg = "PING " + pingsetaddr + " " + pingsetsize + " bytes of data.;"
    pingactive = 1;
    
    PingSubmit();
}


function IPCheck()
{
    //检查传递参数的合法性
    var address = $("#notaddress").val();
    address = address.replace( /(^\s*)|(\s*$)/g, '');
    if (isBlank(address))
    {
        parent.paramFailTip(IDC_NETCHECK_ADDR_FAIL);
        window.focus();
        return false;
    }

    var myRegExp = new RegExp("^\\d+$");        //匹配数字
    var count = $("#notcount").val();
    if ((myRegExp.test(count) == false) || parseInt(count) > 20 || parseInt(count) < 1)
    {
        parent.paramFailTip(IDC_NETCHECK_COUNT + "(1~20)");
        window.focus();
        return false;
    }
    
    var size = $("#notsize").val();
    if ((myRegExp.test(size) == false) || parseInt(size) > 1472 || parseInt(size) < 8)
    {
        parent.paramFailTip(IDC_NETCHECK_SIZE + "(8~1472)");
        window.focus();
        return false;
    }
        
    pingsetaddr = address;
    pingcnt = count;
    pingsetcnt = count;
    pingsetsize = size;
    window.focus();
    return true;
}