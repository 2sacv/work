<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

<link href="/css/base.css" type="text/css" rel="stylesheet"/>
<link href="/css/form.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript" src="/js/serialport.js"></script>
</head>
<body>
    <div class="left">
        <table border=0 cellPadding=3 cellSpacing=1 class="mainTable">
             <tr><td colspan="2" valign="middle" height="20px" align="left"><b>
               <script>dwn(IDC_PORTSETTING)</script>
             </b></td></tr>
             <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td width="35%" class="caption"><script>dwn(IDC_SERIALPORT)</script></td>
                <td align="left" width="65%">
                  <select id="comtype" style="width:150px" >
                      <option value="0">RS232</option>
                      <option value="1">RS485</option>
                  </select>
                </td>
              </tr>
               <tr>
                <td class="caption"><script>dwn(IDC_BAUDRATE)</script></td>
                <td align="left">
                   <select id="baudrate" style="width:150px" >
                      <option value="300" selected="selected">300</option>
                      <option value="600">600</option>
                      <option value="1200">1200</option>
                      <option value="2400">2400</option>
                      <option value="4800">4800</option>
                      <option value="9600">9600</option>
                      <option value="19200">19200</option>
                      <option value="38400">38400</option>
                      <option value="57600">57600</option>
                      <option value="115200">115200</option>
                  </select>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_DATABIT)</script></td>
                <td align="left">
                  <select id="databits" style="width:150px" >
                        <option value="5" selected="selected">5</option>
                        <option value="6">6</option>
                        <option value="7">7</option>
                        <option value="8">8</option>
                    </select>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_STOPBIT)</script></td>
                <td align="left">
                  <select id="stopbits" style="width:150px"  >
                        <option value="1" selected="selected">1</option>
                        <option value="2">2</option>
                    </select>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_PARITYBIT)</script></td>
                <td align="left">
                  <select id="checktype" style="width:150px"  >
                        <option value="N" selected="selected">
                            <script>dwn(IDC_PARITYBIT_NOTHING);//无</script>
                        </option>
                        <option value="O"><script>dwn(IDC_PARITYBIT_ODD);//奇校验</script></option>
                        <option value="E"><script>dwn(IDC_PARITYBIT_EVEN);//偶校验</script></option>
                        <option value="S"><script>dwn(IDC_PARITYBIT_SPACE);//空格</script></option>
                    </select>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_PROTOCOLNAME)</script></td>
                <td align="left">
                  <select id="selProtocol" style="width:150px"  >
                    </select>
                </td>
              </tr>
              <tr>
                <td class="caption"><script>dwn(IDC_MACHINEADDRESS)</script></td>
                <td align="left">
                   <select id="selAddr" style="width:150px"  >
                    </select>
                </td>
              </tr>
              <tr><td class="hline" colspan="2"></td></tr>
              <tr>
                <td colspan="2"  align="left">
                    <button  class="BtnConfig" onclick="SaveSerialPort();"><script>dwn(IDC_SAVE)</script></button>
                </td>
              </tr>
            </table>
    </div>
</body>
</html>