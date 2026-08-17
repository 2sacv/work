<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

<link href="/css/bootstrap.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery-ui-1.10.4.css" type="text/css" rel="stylesheet"/>
<link href="/css/jquery.ipaddress.css" type="text/css" rel="stylesheet"/>
<link href="/css/index.css" type="text/css" rel="stylesheet"/>
<link href="/css/public_css.css" type="text/css" rel="stylesheet"/>

<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery-ui-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/jquery/jquery.caret.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/jquery/jquery.ipaddress.js"></script>
<script type="text/javascript" src="/js/networksetting.js"></script>

<style type="text/css">
  span { display:-moz-inline-box; display:inline-block; width:150px;text-align:right;}
  .wlanDiv{border:1px solid black;height:400px;width:400px;overflow-y:auto;}
  .wlanDiv div{margin-top:15px;}
  .wlanDiv div label{margin-left:10px;display:inline-block; width:180px;height:25px;line-height: 25px;}
  .wlanDiv div label:hover{background:#888;}
</style>
</head>

<body style="background: #2C2C2C;width:99%;height:100%">
    <div style='background: #3C3D3D;'>
      <div id="tabs" class='tabs ui-tabs ui-widget ui-widget-content ui-corner-all' style='width: 100%;'>
        <ul>
          <li><a href="#tabs-1" id="tabEthnet"><script>dwn(IDC_ETHNET)</script></a></li>
          <li id="litabWlan"><a href="#tabs-0" id="tabWlan"><script>dwn(IDC_BELL_SETTING)</script></a></li>
          <li id="litab4G" style="display:none;"><a href="#tabs-8" id="tab4G"><script>dwn(IDC_4G_STATUS_SET)</script></a></li>
          <li id="litabPppos" style="display:none;"><a href="#tabs-2" id="tabPppos"><script>dwn(IDC_PPPOE)</script></a></li>
          <li id="litabDdns"  style="display:none;"><a href="#tabs-3" id="tabDdns" ><script>dwn(IDC_DDNS)</script></a></li>
          <li id="litabPort"><a href="#tabs-4" id="tabPort" ><script>dwn(IDC_NETPORT)</script></a></li>
          <li><a href="#tabs-5" id="tabEmail" ><script>dwn(IDC_EMAIL_SETTING)</script></a></li>
          <li id="liFtp" style="display:none;" ><a href="#tabs-6" id="tabFtp" ><script>dwn(IDC_FTPCLI_SETTING)</script></a></li>
          <li><a href="#tabs-7" id="tabNetCheck" ><script>dwn(IDC_NETCHECK)</script></a></li>
        </ul>

        <div id="tabs-1">
            <table style='width: 100%;margin-top:5px;'>
              <tr>
                <td  align="left">
                  <script>dwn(IDC_ETHNETSETTING);//DNS参数设置 </script>
                  <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                </td>
              </tr>
              <tr>
                  <td  height="25" align="left">
                    <span ><script>dwn(IDC_DNSADDRESS);//DNS&nbsp;地址</script></span>
                  <input name="form[dns]" id="dns"  type="text" style="width:180px;" class='sysinput'/>
                  </td>
              </tr>
              <tr style="height:20px;">
                <td  ></td>
              </tr>
              <tr>
                <td  align="left">
                  <script>dwn(IDC_CARDSETTING_ETH);//以太网卡设置</script>
                  <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                </td>
              </tr>
              <tr>
                  <td  align="left">
                    <span ><script>dwn(IDC_IPADDRESS);//IP&nbsp;地址</script></span>
                      <input name="form[ip]" id="ip"  type="text" class='sysinput' style="width:180px;"/>
                    </td>
                </tr>
                <tr>
                    <td height="25" align="left">
                        <span ><script>dwn(IDC_SUBNETMASK);//子网掩码</script></span>
                        <input name="form[mask]" id="mask"  class="ip sysinput" class='sysinput' type="text" style="width:180px;"/>
                    </td>
                </tr>
                <tr>
                    <td height="25" align="left">
                      <span ><script>dwn(IDC_GATEWAY);//网关</script></span>  
                      <input name="form[gate]" id="gate"  type="text"  style="width:180px;"/>
                    </td>
                </tr>
                <tr>
                    <td height="25" align="left">
                      <span ><script>dwn(IDC_MACADDRESS);//MAC地址</script></span>
                      <input type="text" id="macaddr"  disabled="disabled" style="width:174px;" class="sysinput2">
                    </td>
                </tr>
                <tr>
                  <td height="25" align="left">
                    <span ><script>dwn(IDC_MTU);//MTU值</script></span>
                    <input type="text" id="ethmtu"  style="width:174px;" class="sysinput2" maxlength="4" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);" onfocus="$('#ethmtu_txt').show();" 
                                  onblur="$('#ethmtu_txt').hide();if(this.value < 1200 || this.value > 1500){alert(IDC_PARAM_ERROR);this.value = 1500;}">
                    <span id="ethmtu_txt" style="display: none;">1200 <= MTU <= 1500</span>
                  </td>
                </tr>
                <tr style="height:20px;">
                  <td  ></td>
                </tr>
                <tr id="trAutoIp" style="display:none;">
                    <td height="25"  align="left">
                      <span ><script>dwn(IDC_AUTOIPSWITCH);//IP自适应</script></span>
                      <input type="checkbox" id="autoipSwitch" name="autoipSwitch" onclick="switchAutoIp();">
                    </td>
                </tr>
                <tr>
                    <td height="25"  align="left">
                      <span ><script>dwn(IDC_DHCPSWITCH);//DHCP开关</script></span>
                      <input type="checkbox" id="dhcpSwitch" name="dhcpSwitch"  onclick="switchDhcp();">
                    </td>
                </tr>
                <tr>
                    <td height="25"  align="left">
                      <span ><script>dwn(IDC_NRETICLE);//非标准长网线</script></span>
                      <input type="checkbox" id="nstdReticle" name="nstdReticle"  onclick="switchNreticle();">
                    </td>
                </tr>
                <tr>
                  <td align="left">
                    <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                      <button style="width:65px;margin-left: 180px" class="btn btn-inverse btn-black button_test index_btn" onclick="SaveEthnet()"><script>dwn(IDC_SAVE)</script></button>
                  </td>
                </tr>
            </table>
        </div>
        
        <div id="tabs-0">
            <table>
              <tr>
                <td>
                   <div class="wlanDiv">
                    <div>
                      <span><script>dwn(IDC_WLAN);</script></span>
                      <input type="text" id="wlan" class="sysinput" style="width:180px;margin-left:0px;"
                           maxlength="63">
                    </div>
                    <div>
                      <span><script>dwn(IDC_WLAN_PASSOWRD);</script></span>
                             <input type="password" id="wlanpassword" class="sysinput" style="width:180px;margin-left:0px;"
                           maxlength="63">
                           <input type="checkbox" id="showWlanPwd" onclick="switchWlanPwdShow();" style="margin-left:5px">
                     </div>
                    <div>
                      <span><script>dwn(IDC_PTZ_mode);</script></span>
                              <select id="wlanmode"  style="width:180px;margin-left:0px;" class="sysinput2">
                                  <option value="0">AP</option>
                                  <option value="1">AP+Station</option>
                              </select>
                      </div>
                     <div>
                      <span><script>dwn(IDC_IP_TYPE);</script></span>
                              <select id="iptypesel"  style="width:180px;margin-left:0px;" class="sysinput2">
                                 <!-- <option value="0"><script>dwn(IDC_IP_MANUAL);</script></option> -->
                                  <option value="1"><script>dwn(IDC_IP_AUTO_DHCP);</script></option>
                              </select>
                      </div>
                      <div id="tr0">
                        <span><script>dwn(IDC_IPADDRESS);</script></span>
                             <input type="text" id="wlandhcpip"  disabled="disabled" class='sysinput'  style="width:180px;margin-left:0px;"/>
                      </div>
                      <div id="tr1">
                        <span><script>dwn(IDC_IPADDRESS);</script></span>
                              <input name="form[wlanip]" id="wlanip"  type="text" class='ip sysinput' style="width:180px;margin-left:15px;"/>
                      </div>
                      <div id="tr2">
                        <span><script>dwn(IDC_SUBNETMASK);</script></span>
                              <input name="form[wlanmask]" id="wlanmask"  class="ip sysinput"  type="text" style="width:180px;"/>
                      </div>
                      <div id="tr3">
                        <span><script>dwn(IDC_GATEWAY);</script></span>
                             <input name="form[wlangate]" id="wlangate"  type="text" class='ip sysinput'  style="width:180px;"/>
                      </div>
                      <div id="tr4">
                        <span><script>dwn(IDC_MACADDRESS);</script></span>
                             <input type="text" id="wlanmacaddr"  disabled="disabled" class='sysinput'  style="width:180px;margin-left:0px;"/>
                      </div>
                      <div>
                        <span><script>dwn(IDC_PPPOE_STATUS);</script></span>
                             <label id="wlanStatus"></label>
                      </div>
                      <div align="center">
                            <button style="width:65px;" class="btn btn-inverse btn-black index_btn" onclick="SaveWlan();"><script>dwn(IDC_SAVE);</script></button>
                            <button style="width:65px;" class="btn btn-inverse btn-black index_btn" onclick="refreshWlanStatus();"><script>dwn(IDC_REFRESH);</script></button>
                        </div>
                    </div>
                </td>
                <td width="30px;"></td>
                <td>
                   <div class="wlanDiv" id="wlsnTable">
                          <button  class="btn btn-inverse btn-black index_btn" style="width:65px;" onclick="SearchWLAN();" id="btnSearchWlan"><script>dwn(IDC_SEARCH);</script></button>
                    </div>
                </td>
              </tr>
            </table>
        </div>
        <div id="tabs-8">
            <table>
              <tr>
                <td height="25" align="left">
                      <span><script>dwn(IDC_MODE);</script></span>
                      <input type="radio" name="4g_mode_switch" value="1"  onclick="Set4GMode(1);">
                      <script>dwn(IDC_GEN_SWITCH_OPEN);</script>
                      <input type="radio" name="4g_mode_switch" value="0" onclick="Set4GMode(0);">
                      <script>dwn(IDC_GEN_SWITCH_CLOSE);</script>
                  </td>
              </tr>
              <tr>
                <td height="25" align="left">
                      <span><script>dwn(IDC_SIM_INFO);</script></span>
                          <input  id="sim_info" class="sysinput" style="width:180px;margin-left:0px;"
                           disabled>
                  </td>
              </tr>
              <tr>
                <td height="25" align="left">
                      <span><script>dwn(IDC_IF_MODE_4G);</script></span>
                            <input  id="is_4g" class="sysinput" style="width:180px;margin-left:0px;"
                           disabled>
                  </td>
              </tr>
              <tr>
                <td height="25" align="left">
                      <span><script>dwn(IDC_Signal_strength);</script></span>
                            <input  id="dbm" class="sysinput" style="width:180px;margin-left:0px;"
                           disabled>
                  </td>
              </tr>
              <tr style="display:none;">
                <td height="25" align="left">
                        <span><script>dwn(IDC_UPLINK_SPEED);</script></span>
                             <input  id="txBpsec" class="sysinput" style="width:180px;margin-left:0px;"
                           disabled>
                  </td>
              </tr>
              <tr style="display:none;">
                <td height="25" align="left">
                        <span><script>dwn(IDC_DOWNWARD_SPEED);</script></span>
                              <input  id="rxBpsec" class="sysinput" style="width:180px;margin-left:0px;"
                           disabled>
                  </td>
              </tr>
                <td height="25" align="left">
                        <span><script>dwn(IDC_IPADDRESS);</script></span>
                              <input  id="4g_ip" class="sysinput" style="width:180px;margin-left:0px;"
                           disabled>
                  </td>
              </tr>
              <tr>
                <td height="25" align="left">
                  <span><script>dwn("");</script></span>
                          <button style="width:65px;" class="btn btn-inverse btn-black index_btn" onclick="refresh4GStatus();"><script>dwn(IDC_REFRESH);</script></button>
                </td>
              </tr>
            </table>
        </div>
        <div id="tabs-2">
          <table style='width: 100%;' >
              <tr height="30">
                  <td  align="left">
                      <span>
                        <script>dwn(IDC_PPPOE_SWITCH);//PPPOE开关</script>
                      </span>
                      <input type="radio" id="pppoe_open" name="pppoeswitch" value='1' onclick="pppoeEnable(true);" style='margin-left: 15px;'>
                      <label for="pppoe_open"><script>dwn(IDC_GEN_SWITCH_OPEN);//开</script></label>
                      <input type="radio" id="pppoe_close" name="pppoeswitch" value='0' onclick="pppoeEnable(false);">
                      <label for="pppoe_close"><script>dwn(IDC_GEN_SWITCH_CLOSE);//关</script></label>
                  </td>
              </tr>
              <tr height="30">
                  <td align="left">
                    <span><script>dwn(IDC_NETWORKCARDSELECT);//网卡选择</script></span>
                    <select id="nicsel" style="width:150px;">
                        <option value="0">ETH</option>
                        <!-- <option value="1">WIFI</option> -->
                    </select>
                </td>
              </tr>
              <tr height="30">
                  <td align="left">
                      <span><script>dwn(IDC_PPPOE_USRNAME);//PPPOE用户名</script></span>
                      <input type="text" class='sysinput' id="pppoeuser" maxlength="32" style="width:150px;" />
                  </td>
              </tr>
              <tr height="30">
                  <td align="left">
                      <span><script>dwn(IDC_PPPOE_PASSWD);//PPPOE密码</script></span>
                      <input id="pppoepasswd" class='sysinput' type="password" maxlength="32" style="width:150px;" >
                  </td>
              </tr>
              <tr>
                <td align="left">
                  <span><script>dwn(IDC_PPPOE_STATUS);//连接状态</script></span>
                  <font  id="pppoestatus"></font>
                </td>
              </tr>
              <tr id="trPpppoeIp">
                <td align="left">
                  <span><script>dwn(IDC_PPPOE_IP);</script></span>
                  <font  id="pppoeip"></font>
                </td>
              </tr>
                <tr>
                  <td align="left">
                    <div style="margin:5px 5px 5px 0;border:1px solid #666"></div>
                    <button style="width:65px;margin-left: 150px" class="btn btn-inverse btn-black button_test index_btn" onclick="FlushPppoe()"><script>dwn(IDC_FLUSH)</script></button>
                      <button style="width:65px;" class="btn btn-inverse btn-black button_test index_btn" onclick="SavePppoe()"><script>dwn(IDC_SAVE)</script></button>
                  </td>
                </tr>
          </table>
        </div>
        <div id="tabs-3">
            <table style='width: 100%;'>
              <tr>
                <td align="left">
                  <span>
                    <script>dwn(IDC_9299DDNS_DOMAIN);//DDNS运营列表</script></span>
                
                    <select onchange="changeDdnsSupport();" id="ddnssupport" style="width:200px;">
                        <option value = "3322">3322.org</option>
                        <option value = "9299">9299.org</option>
                    </select>
                </td>
              </tr>

              <tr>
                <td align="left">
                    <span><script>dwn(IDC_PPPOE_SWITCH)</script></span>
                    <input type="radio" id="ddns_open" name="ddnsswitch" value='1' onclick="ddnsEnable(true);" style='margin-left: 15px;'>
                      <label for="ddns_open"><script>dwn(IDC_GEN_SWITCH_OPEN);//开</script></label>
                      <input type="radio" id="ddns_close" name="ddnsswitch" value='0' onclick="ddnsEnable(false);">
                      <label for="ddns_close"><script>dwn(IDC_GEN_SWITCH_CLOSE);//关</script></label>
                </td>
              </tr>
              <tr>
                <td align="left">
                  <span><script>dwn(IDC_PPPOE_USRNAME);//用户名</script></span>
                  <input  id="ddnsusername" class='sysinput' type="text" class='sysinput' maxlength="32" style="width:200px;" >
                </td>
              </tr>
              <tr>
                <td align="left">
                  <span><script>dwn(IDC_PPPOE_PASSWD);//密码</script></span>
                  <input  id="ddnspassword" class='sysinput' type="password" maxlength="32" style="width:200px;" >
                </td>
              </tr>
              <tr id="3322addr">
                <td align="left">
                  <span><script>dwn(IDC_SELECTED_ADDR);//DDNS Addr</script></span>
                  <input  id="ddnsaddrs" type="text" class='sysinput' maxlength="32" style="width:200px;" >
                </td>
              </tr>
              <tr>
                <td align="left">
                  <span><script>dwn(IDC_PPPOE_STATUS);//连接状态</script></span>
                  <font  id="ddnsstatus"></font>
                </td>
              </tr>
              <tr>
                  <td align="left"> 
                      <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                      <button style="width:65px;margin-left: 180px" class="btn btn-inverse btn-black button_test index_btn" onclick="SaveDdns()"><script>dwn(IDC_SAVE)</script></button>
                  </td>
                </tr>
          </table>
        </div>
        <div id="tabs-4">
          <table style='width: 100%;'>
             <tr>
                <td align="left">
                    <span><script>dwn(IDC_WEB_PORT);//Web端口</script></span>
                    <input id="porthttp" class='sysinput' type="text" maxlength="5" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;
                    <label for='httpen' id="lahttpen"><script>dwn(IDC_UPNP_SWITCH);//UPNP开关</script></label>
                    <input type="checkbox" name="httpen" id="httpen" onclick="webPort()">
                    <font id="webPort_text" name="webPort_text" style="display:none"></font>
                </td>
            </tr>
            <tr  style="display:none">
                <td width="left">
                    <span><script>dwn(IDC_FTP_PORT);//Ftp端口</script></span>
                    <input id="portftp" class='sysinput' type="text" maxlength="5" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;
                    <label for='ftpen'><script>dwn(IDC_UPNP_SWITCH);//UPNP开关</script></label>
                    <input type="checkbox" name="ftpen" id="ftpen" onclick="frpPort()">
                    <font id="ftpPort_text" name="ftpPort_text" style="display:none"></font>
                </td>
            </tr>
            <tr  style="display:none">
                <td align="left">
                  <span><script>dwn(IDC_RTSP_PORT);//RTSP端口</script></span>
                  <input id="portrtsp" class='sysinput' type="text" maxlength="5" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;
                  <label for='rtspen'  id="lartspen"><script>dwn(IDC_UPNP_SWITCH);//UPNP 开关</script></label>
                  <input type="checkbox" name="rtspen" id="rtspen" onclick="rtspPort()">
                  <font id="rtspPort_text" name="rtspPort_text" style="display:none"></font>
                </td>
            </tr>
            <tr style="display:none">
                <td align="left">
                  <span><script>dwn(IDC_SPEAK_PORT);//对讲端口</script></span>
                    <input id="portvoice" class='sysinput' type="text" maxlength="5" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;
                    <label for='voiceen'><script>dwn(IDC_UPNP_SWITCH);//UPNP 开关</script></label>
                    <input type="checkbox" name="voiceen" id="voiceen" onclick="speakPort()">
                    <font id="speakPort_text" name="speakPort_text" style="display:none"></font>
                </td>
            </tr>
            <tr style="display:none">
                <td align="left">
                 <span> <script>dwn(IDC_UPDATE_PORT);//升级端口</script></span>
                    <input class='sysinput' id="portupdate" type="text" maxlength="5" style="width:150px;" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;
                    <label for='updateen'><script>dwn(IDC_UPNP_SWITCH);//UPNP 开关</script></label>
                    <input type="checkbox" id="updateen" onclick="updatePort()">
                    <font id="updatePort_text" style="display:none"></font>
                </td>
            </tr>
            <tr>
                <td align="left">
                  <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                  <button style="width:65px;margin-left: 180px" class="btn btn-inverse btn-black button_test index_btn" onclick="SavePort()"><script>dwn(IDC_SAVE)</script></button>
                </td>
            </tr>
          </table>
        </div>
        <div id="tabs-5">
          <table style='width: 100%;'>
            <tr>
              <td align="left">
                  <span><script>dwn(IDC_EMAIL_SERVER);//服务器</script></span>
                  <input type="text" class='sysinput' id="emailserver" style="width:250px;" maxlength="63" >
              </td>
            </tr>
            <tr>
                <td align="left">
                  <span><script>dwn(IDC_EMAIL_USERNAME);//用户名</script></span>
                    <input type="text" class='sysinput' id="emailuser" style="width:250px;" maxlength="31" >
                </td>
            </tr>
            <tr>
                <td align="left">
                  <span><script>dwn(IDC_EMAIL_PASSWORD);//密码</script></span>
                    <input type="password" class='sysinput' id="emailpasswd" style="width:250px;" maxlength="31" >
                </td>
            </tr>
            <tr>
                <td align="left">
                  <span><script>dwn(IDC_EMAIL_TOADDR);//目的</script></span>
                    <input type="text" class='sysinput' id="emailtoaddr" style="width:250px;" maxlength="63" >
                </td>
            </tr>
            <tr>
                <td align="left">
                  <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                  <button style="width:65px;margin-left: 180px" class="btn btn-inverse btn-black button_test index_btn" onclick="SaveEmail()"><script>dwn(IDC_TEST)</script></button>
                </td>
            </tr>
          </table>
        </div>
        <div id="tabs-6">
          <table style='width: 100%;'>
            <tr>
                <td align="left">
                  <span><script>dwn(IDC_FTPCLI_SERVER);//服务器</script></span>
                    <input type="text" class='sysinput' id="ftpserver" style="width:250px;" maxlength="63" >
                </td>
            </tr>
            <tr>
                <td align="left">
                  <span><script>dwn(IDC_EMAIL_USERNAME);//用户名</script></span>
                    <input type="text" class='sysinput' id="ftpuser" style="width:250px;" maxlength="31" >
                </td>
            </tr>
            <tr>
                <td align="left">
                  <span><script>dwn(IDC_FTPCLI_PASSWORD);//密码</script></span>
                    <input type="password" class='sysinput' id="ftppasswd" style="width:250px;" maxlength="31" >
                </td>
            </tr>
            <tr>
                <td align="left">
                   <span><script>dwn(IDC_FTPCLI_PATH);//路径</script></span>
                    <input type="text" class='sysinput' id="ftppath" style="width:250px;" maxlength="63" >
                </td>
            </tr>
            <tr>
                <td align="left">
                  <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                  <button style="width:65px;margin-left: 180px" class="btn btn-inverse btn-black index_btn" onclick="SaveFtp()"><script>dwn(IDC_SAVE)</script></button>
                </td>
            </tr>
          </table>
        </div>
        <div id="tabs-7">
          <table style='width: 100%;'>
               <tr align="left">
                 <td>
                   <script>dwn(IDC_NETCHECK_SETTINGS);</script>
                  <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                 </td>
               </tr>
               <tr>
                  <td align="left">
                    <span><script>dwn(IDC_NETCHECK_ADDR);</script></span>
                    <input type="text" class='sysinput' id="notaddress" value="192.168.1.211" >
                  </td>
                </tr>
                <tr>
                  <td align="left">
                    <span><script>dwn(IDC_NETCHECK_COUNT);</script></span>
                    <input type="text" class='sysinput' id="notcount" style="width:100px;" value="4" maxlength="3" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;<font >(1~20)</font>
                  </td>
                </tr>
                <tr>
                  <td align="left">
                    <span><script>dwn(IDC_NETCHECK_SIZE);</script></span>
                    <input type="text" class='sysinput' id="notsize" style="width:100px;" value="16" maxlength="5" onKeyPress="event.returnValue=IsDigit();" onkeyup="IsDigitUp(this);">&nbsp;&nbsp;<font >(8~1472)</font>
                  </td>
                </tr>
                <tr>
                <td align="left">
                    <span>&nbsp;</span>&nbsp;
                    <button style="width:65px;" class="btn btn-inverse btn-black button_test index_btn" onclick="Ping()"><script>dwn(IDC_GEN_IPCHECK_SEND)</script></button>
                </td>
                </tr>
                <tr align="left">
                 <td>
                   <script>dwn(IDC_NETCHECK_RESULT);</script>
                  <div style="margin:5px 5px 5px 0px;border:1px solid #666"></div>
                 </td>
               </tr>
              <tr align="left">
                 <td>
                   <textarea id="notresult" class='centent' readonly="readonly" >
                   </textarea>
                 </td>
               </tr>
               <tr>
                 <td>
                  <iframe name="frmsubmit" id="frmsubmit" width="100%" height="210"  frameborder="0" scrolling="no" style="display:none;">
                  </iframe>
                 </td>
               </tr>
          </table>
        </div>
        
  </div>
</div>
</body>
</html>
