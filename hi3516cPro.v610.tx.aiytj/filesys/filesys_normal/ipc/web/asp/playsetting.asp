<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript">
    function savePlaySetting(){
        window.parent.savePlaySetting();
    }

    $(function(){
        var mode = GetCookieByKey("playMode");
        mode = mode== -1?2:mode;
        $("input[name='playMode'][value="+mode+"]").attr("checked",true);

        var type = GetCookieByKey("ljtypes");
        type = type== -1?1:type;
        $("input[name='typelj'][value="+ type +"]").attr("checked",true);
    });
</script>
</head>

<body style="background-color:#2C2C2C;color:white">
    <table style="width:100%;height:100%;font-size:10pt;background:#2C2C2C;border-collapse:collapse;border-color: gray;">
        <tr>
            <td colspan="2" align="center">
                <script>dwn(IDC_PLAYMODE_PLAYBACKOPTION);//播放选项</script>
            </td>
        </tr>
        <tr>
            <td align="left">
                <script>dwn(IDC_PLAYMODE_TITLE)</script></td>
            <td align="left">
                <input type="radio" name="playMode" id="playFluent" value="3">
                <script>dwn(IDC_PLAYMODE_FLUENT)</script>
                <input type="radio" name="playMode" id="playModerate" value="2">
                <script>dwn(IDC_PLAYMODE_Moderate)</script>
                <input type="radio" name="playMode" id="playRealtime" value="1">
                <script>dwn(IDC_PLAYMODE_REALTIME)</script>
            </td>
        </tr>
        <tr>
            <td align="left">
                <script>dwn(IDC_CONNECT_TYPE)</script></td>
            <td align="left">
                <input type="radio" name="typelj" value="1">TCP
                <input type="radio" name="typelj" value="0">UDP
            </td>
        </tr>
        <tr>
            <td colspan="2" align="center">
                <button style="width:65px;height:35px;background:#000;border:none;color:white;"  onclick="savePlaySetting();"><script>dwn(IDC_SAVE)</script></button>
            </td>
        </tr>
    </table>
</body>
</html>
