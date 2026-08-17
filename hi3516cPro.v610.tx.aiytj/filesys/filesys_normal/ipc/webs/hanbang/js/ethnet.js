$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        /*GetJCP({cmd: "version -act list", ParseJCP: function(jcpGet){
        if(jcpGet !== "Error"){
            platform = jcpGet.platform.toLowerCase();
            if(platform.indexOf('hb') == 0){
                $("#trAutoIp").hide();
            }
        }}});*/
        GetJCP({cmd: "ethcfg -act list", ParseJCP: ParseEthcfg});
    }
})

var ports = 80;
var oldip = "192.168.1.217";
function ParseEthcfg(jcpobj)
{
    try
    {
        $("#dns").val(jcpobj.dns);
        $("#dns").ipaddress({cidr:false});
        
        $("#ip").val(jcpobj.ethip);
        $("#ip").ipaddress({cidr:false});
        oldip = jcpobj.ethip;
    
        $("#mask").val(jcpobj.ethmask);
        $("#mask").ipaddress({cidr:false});
        
        $("#gate").val(jcpobj.ethgw);
        $("#gate").ipaddress({cidr:false});
        
        $("#macaddr").val(jcpobj.ethmac);
        
        $("#mtu").val(jcpobj.mtu);

        $("#dhcpSwitch").prop("checked", jcpobj.ethdhcp==1?true:false);
        $("#autoipSwitch").prop("checked", jcpobj.ipadaen==1?true:false);
        
        
        switchDhcp();
    }
    catch(e){ return e; }
}
function SaveEthnet()
{
    var dnsaddr = $("#dns").getip();
    var ip = $("#ip").getip();
    var submask = $("#mask").getip();
    var gateway = $("#gate").getip();
    var mtu = $("#mtu").val();
    var dhcpen = $('#dhcpSwitch').prop('checked')?1:0;
    var autoipen = $('#autoipSwitch').prop('checked')?1:0;

    //判断DNS , IP , SUBMASK, GATEWAY是否为0-255之间的数
    for (var i = 0; i < 4; i ++)
    {
        if (dnsaddr.split(".")[i]=='' || dnsaddr.split(".")[i] > 255 || dnsaddr.split(".")[i] < 0
            ||ip.split(".")[i]=='' || ip.split(".")[i] > 255 || ip.split(".")[i] < 0
            ||submask.split(".")[i]=='' || submask.split(".")[i] > 255 || submask.split(".")[i] < 0
            ||gateway.split(".")[i]=='' ||gateway.split(".")[i] > 255 || gateway.split(".")[i] < 0)
        {
            parent.paramFailTip(IDC_MSGBOX_ADDRESS_NUM);
            window.focus();
            return;
        }
    }
    
                                    
    //判断 MTU 值是否在范围之内   
    if (mtu > 1500 || mtu < 1300) 
    {                             
        alert(IDC_MSGBOX_MTU);    
        window.focus();           
        return;                   
    }                             

    //判断IP地址与网关是否在同一网段
    var ret = Ip_Gateway_Chk(ip, gateway, submask);
    if (ret == false)
    {
        //不在同一网段，直接返回
        parent.paramFailTip(IDC_MSGBOX_IPGATE_DIFFER);
        window.focus();
        return;
    }
    var ipad = ip.split(".");
    if(ipad[3] == "")
    {
        parent.paramFailTip(IDC_IP_ADD);
        window.focus();
        return;
    }

    try
    {
        var jcpstr = "ethcfg -act set -ethip " + ip + " -ethmask " + submask + " -ethgw " + gateway + " -dns " + dnsaddr +" -ethdhcp " + dhcpen + " -ipadaen "+autoipen;
        jcpstr += " -mtu " + $("#mtu").val();
        GetJCP({cmd: jcpstr, ParseJCP: function(jcpstr)
        {
            if(jcpstr != "Error")
            {
                parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
                window.focus();
                var ipaddr = (document.URL.split('//')[1]).split('/')[0].split(':')[0];
                if (ipaddr == oldip && oldip != ip)
                {
                    parent.location.href = "http://" + ip + ":" + ports;
                }
            }
            else
            {
                parent.paramFailTip(IDC_MASK_ERROR);
                window.focus();
            }
        }, async: false});
    }
    catch(E){ return E; }
}

//判断页面与网关的地址是否在同一网段functions 方法：
//IP and 掩码  是否等于 网关 and 掩码
function Ip_Gateway_Chk(ip, gateway, submask)
{
    var ip = ip.split(".");
    var gateway = gateway.split(".");
    var submask = submask.split(".");
    var ip_sub = new Array(), gateway_sub = new Array();
    
    for (var i = 0; i < ip.length; i ++)
    {
        ip_sub[i] = ip[i] & submask[i];
        gateway_sub[i] = gateway[i] & submask[i];
    }
    
    for (var i = 0; i < ip_sub.length; i ++)
    {
        if (ip_sub[i] != gateway_sub[i]) return false;
    }
    return true;
}

function switchDhcp(){
    var flag = $('#dhcpSwitch').prop('checked');
    $("#ip").enableip({enable:!flag});
    $("#mask").enableip({enable:!flag});
    $("#gate").enableip({enable:!flag});
	$("#dns").enableip({enable:!flag});
    if(flag){
        $('#autoipSwitch').prop('checked',false);
    }
}

function switchAutoIp(){
    var flag = $('#autoipSwitch').prop('checked');
    if(flag){
        $('#dhcpSwitch').prop('checked',false);
        parent.paramSaveTip(IDC_AUTOIP_TIP);
        switchDhcp();
    }
}
