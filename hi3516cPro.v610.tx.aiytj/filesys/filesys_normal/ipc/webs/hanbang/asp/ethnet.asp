<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery.ipaddress.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/jquery.caret.js"></script>
<script type="text/javascript" src="/jquery/jquery.ipaddress.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/ethnet.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_ETHNETSETTING)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td class="caption"><script>dwn(IDC_DNSADDRESS)</script></td>
                <td align="left">
                   <input  id="dns" name="form[dns]" type="text"/>
                </td>
              </tr>
              <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_CARDSETTING_ETH)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td class="caption"><script>dwn(IDC_IPADDRESS)</script></td>
                <td align="left">
                   <input  id="ip" name="form[ip]" type="text"/>
                </td>
              </tr>

              <tr>
                <td class="caption"><script>dwn(IDC_SUBNETMASK)</script></td>
                <td align="left">
                   <input  id="mask" name="form[mask]" type="text"/>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_GATEWAY)</script></td>
                <td align="left">
                   <input  id="gate"  name="form[gate]"  type="text"/>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_MACADDRESS)</script></td>
                <td align="left">
                   <input type="text" id="macaddr"  disabled="disabled" style="width:175px;height:20px;border:1px solid #222;line-height:20px;" >
                </td>
            <tr>
              <td width="10%" class="caption">
                <script>dwn(IDC_MTU);//MTU</script>
              </td>
              <td width="15%" align="left">
                  <input id="mtu" type="text" class="sysinput" style="width:100px;"  maxlength="4">
                   <font id="mtu">(1300~1500)</font>
              </td>
            </tr>
               <tr style="height:20px;">
                  <td colspan="2"></td>
                </tr>
                <tr id="trAutoIp">
                    <td class="caption"><script>dwn(IDC_AUTOIPSWITCH)//IP自适应</script></td>
                    <td align="left">
                      <input type="checkbox" id="autoipSwitch" name="autoipSwitch" onclick="switchAutoIp();">
                    </td>
                </tr>
                <tr>
                    <td class="caption"><script>dwn(IDC_DHCPSWITCH);//DHCP开关</script></td>
                    <td align="left">
                      <input type="checkbox" id="dhcpSwitch" name="dhcpSwitch"  onclick="switchDhcp();">
                    </td>
                </tr>
              <tr>
                <td colspan="2"  align="left">
                    <button  class="BtnConfig" onclick="SaveEthnet();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>
