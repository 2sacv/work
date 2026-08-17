<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />
<title></title>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>


<script language="javascript">
var pingmsg = "";

window.onload = function ()
{
    if (0 >= parent.pingactive)
    {
        parent.pingmsg = "";
        return;
    }
    
    if (0 >= pingmsg.length)
    {
        parent.pingdelay = setTimeout('PingReload()', 1000);
        return;
    }
        
    // add ping result
    if (0 <= parseInt(parent.pingcnt))
    {
        parent.pingmsg += pingmsg;
    }
    
    // add tail
    if (0 == parseInt(parent.pingcnt))
    {
        var temp = parent.pingmsg.split(";");
        var lost = 0;
        var tm_min = 1000000;
        var tm_max = 0,tm_total = 0;
        
        for (var i = 0; i < temp.length; i++)
        {
            if (0 >= temp[i].length)
            {
                continue;
            }
            
            if ("Packet loss" == temp[i])
            {// cal lost packets
                lost++;
                continue;
            }
            
            // get ping time
            var tm = temp[i].split("time=");
            if (2 > tm.length)
            {
                continue;
            }
            
            tm = tm[1].split(" ");
            if (2 > tm.length)
            {
                continue;
            }
            
            tm = tm[0];
            tm_min = tm_min > tm ? tm : tm_min;
            tm_max = tm_max < tm ? tm : tm_max;
            tm_total = parseFloat(tm_total) + parseFloat(tm);
        }
        tm_min = tm_min > tm_max ? tm_max : tm_min;
        
        parent.pingmsg += "\n---- " + parent.pingsetaddr + " ping statistics ----;";
        parent.pingmsg += (parent.pingsetcnt - lost) + " packets transmitted, " + lost + " packet loss;";
        parent.pingmsg += "ping time min/avg/max = " + tm_min + "/" + (tm_total / parent.pingsetcnt).toFixed(1) + "/" + tm_max + " ms;";
    }
        
    // show message
    if (0 <= parseInt(parent.pingcnt))
    {        
        var temp = parent.pingmsg.split(";");
        parent.document.all.notresult.value = "\t";
        
        for (var i = 0; i < temp.length; i++)
        {
            if (0 >= temp[i].length)
            {
                continue;
            }
            parent.document.all.notresult.value += temp[i] + "\n";
        }
    }
    
    if (0 < parseInt(parent.pingcnt))
    {
        parent.pingdelay = setTimeout('PingSubmit()', 2000);
    }
    else
    {
        parent.pingactive = 0;
        parent.pingmsg = "";
        parent.pingcnt--;
    }
}

function PingSubmit()
{
    window.clearTimeout(parent.pingdelay);
    
    document.all.address.value = parent.pingsetaddr;
    document.all.size.value = parent.pingsetsize - 8;
    
    parent.pingcnt--;  
    document.all.netcheck.submit();
}

function PingReload()
{
    location.href = "netchecksub.asp";
}

</script>
</head>

<body>
    <form name="netcheck" id="netcheck" method="post" action="/webs/netchkCfg" style="background:#EDEDED;">
        <textarea name="notresult" id="notresult" style="background:#EDEDED;width:100%;height:160px;color:#000;border:1px solid inset;font-size:8pt;">
        </textarea>
        <table border="0" cellspacing="0" cellpadding="0" width="100%" style="background:#EDEDED;">
            <tr>
                <td height="25"><hr align="center" color="#999999" /></td>
            </tr>    
            <tr>
                <td height="25" align="center">
                    <button class="button" onclick="Ping();" style="width:65px"><script>dwn(IDC_GEN_IPCHECK_SEND)//发送</script></button>
                </td>
            </tr>
        </table>
        <input type="text" name="address" id="address" style="display:none;" value="">
        <input type="text" name="size" id="size" style="display:none;" value="0">
    </form>
</body>
<%outNetCheckConf%>
</html>
