var g_lan_arr = ["简体中文","English"]; //tag_lang_russion, 修改"简体中文"为后面的这个，"русский"
var g_lan_js_arr = ["chinese.js","english.js"]; //tag_lang_russion_js ,修改"chinese.js"为后面的这个，"russian.js"

var g_hostname = (document.URL.split('//')[1]).split('/')[0];
var g_platform = "guobiao,hxht,hngs,tslive,p2p,jstar,tencent,wstk"; //网页支持平台配置

Set_cookie = function(key,value){
   $.cookie(key, value, {path: '/' });
}

// cookie对的搜索
function GetCookieByKey(key)
{
    var str = $.cookie(key)
    if (null == str)
    {
        return -1;
    }
    return str;
}

// 删除cookie
function deleteCookie(key)
{   
    $.cookie(key, null, {path: '/' });
}

Language_sel = function()
{
    var webdeflang = GetCookieByKey("webdeflang");
    if (parseInt(webdeflang) >= 0) {
        if (webdeflang == 2) {
            g_lan_arr[0] = "русский";
            g_lan_js_arr[0] = "russian.js";
        } else if (webdeflang == 1) {
            g_lan_arr.shift();
            g_lan_js_arr.shift();
        }
    }
    var t = GetCookieByKey("languages");
    if (parseInt(t) >= 0) {
        dwn('<script type="text/javascript" src="/language/'+g_lan_js_arr[parseInt(t)]+'"></script>');
    }
}

date = function(){
    a = new_date();
    b = new_time();
    return a+" "+b;
}

new_date = function(){
    var now = new Date();
    var strDate = now.getFullYear() + "-";
    if ((1 + now.getMonth()) < 10)
    {// 前面补零，比如 1 -> 01
        strDate += '0';
    }
    strDate += (1 + now.getMonth()) + "-";
    
    if (now.getDate() < 10)
    {
        strDate += '0';
    }
    strDate += now.getDate();
    delete now;
    now = null;
    return strDate;
}

new_date_dmy = function(){
    var now = new Date();
    var strDate = "";
    if (now.getDate() < 10)
    {
        strDate += '0';
    }
    strDate += now.getDate();
    strDate += "-";
    
    if ((1 + now.getMonth()) < 10)
    {
        strDate += '0';
    }
    strDate += (1 + now.getMonth()) + "-";
    
    strDate += now.getFullYear();
    
    delete now;
    now = null;
    return strDate;
}

new_date_mdy = function(){
    var now = new Date();
    var strDate = "";

    if ((1 + now.getMonth()) < 10)
    {
        strDate += '0';
    }
    strDate += (1 + now.getMonth()) + "-";


    if (now.getDate() < 10)
    {
        strDate += '0';
    }
    strDate += now.getDate();
    strDate += "-";
    
   
    strDate += now.getFullYear();
    
    delete now;
    now = null;
    return strDate;
}

new_time = function(){
    var now = new Date();
    var strDate = "";
    if (now.getHours() < 10)
    {
        strDate += "0";
    }
    strDate += now.getHours() + ":";
    
    if (now.getMinutes() < 10)
    {
        strDate += "0";
    }
    strDate += now.getMinutes() + ":";
    
    if (now.getSeconds() < 10)
    {
        strDate += "0";
    }
    strDate += now.getSeconds();
    delete now;
    now = null;
    return strDate;
}

times = function(v){
    var a = v.length < 2 ? 0+v : v;
    return a;
}

video_times = function(){
    var now = new Date();
    var strDate = "";
    if (now.getHours() < 10)
    {
        strDate += "0";
    }
    strDate += now.getHours();
    
    if (now.getMinutes() < 10)
    {
        strDate += "0";
    }
    strDate += now.getMinutes();
    
    if (now.getSeconds() < 10)
    {
        strDate += "0";
    }
    strDate += now.getSeconds();
    delete now;
    now = null;
    return strDate;
}

function RQcheck(RQ) {
    if (RQ == '')
        return false;
    var date = RQ;
    //var result = date.match(/^(\d{1,4})(-|\/)(\d{1,2})\2(\d{1,2})$/);
    var result = date.match(/^(\d{1,4})(-)(\d{1,2})\2(\d{1,2})$/);
    if (result == null)
        return false;
    var d = new Date(result[1], result[3] - 1, result[4]);
    return (d.getFullYear() == result[1] && (d.getMonth() + 1) == result[3] && d.getDate() == result[4]);

}


// 获取id=arg的对象的绝对位置
function getPosition(arg)                   
{
    var o = document.getElementById(arg);
    var ret={x:0,y:0};
    while(o&&o.offsetParent)
    {
        ret.x += o.offsetLeft + o.offsetParent.clientLeft;
        ret.y += o.offsetTop + o.offsetParent.clientTop;
        o=o.offsetParent;
    }
    
    return ret;
}

function formatDate(date){
    var now = new Date(date);
    var strDate = now.getFullYear() + "-";
    if ((1 + now.getMonth()) < 10)
    {// 前面补零，比如 1 -> 01
        strDate += '0';
    }
    strDate += (1 + now.getMonth()) + "-";
    
    if (now.getDate() < 10)
    {
        strDate += '0';
    }
    strDate += now.getDate();
    delete now;
    now = null;
    return strDate;
}

//输出
function dwn(word){
    document.write(word);
}

//判断str是否为空
function isBlank(str)
{
    if( str !="")
    {
        return false;
    }
    else
    {
        return true;
    }
}

//从字符串中根据key获取值
function GetRtspKeyStr(RtspStr, KeyStr)
{
    var v = -2;
    if (isBlank(RtspStr))
    {
        return v;
    }
    
    var arrKey = RtspStr.split(";");
    for(var i = 0; i < arrKey.length; i++)
    {
        var arr = arrKey[i].split("=");
        var str = arr[0].replace(/^\s*/, "");   // 去掉开始空格
        str = str.replace(/\s*$/, "");          // 去掉结尾空格
        if(KeyStr == str)
        {
            if(arr.length>2)
            {
                v = "";
                for(var j = 1 ; j < arr.length ; j++){
                    v += unescape(arr[j]) +"=";
                }
                v = v.substr(0,v.length-1);
                break;
            }
            else
                v = unescape(arr[1]);
                break;
        }
    }
    return v;
}

function FormatTime(TimeSeconds) 
{
    var temptime;
    var FTime = "";
    var strtime = TimeSeconds % 60;
    if(strtime >= 10)
    {
        FTime = ":" + strtime + FTime;
    }
    else
    {
        FTime = ":0" + strtime + FTime;
    }

    strtime = parseInt(TimeSeconds/60) % 60;
    if(strtime >= 10)
    {
        FTime = ":" + strtime + FTime;
    }
    else
    {
        FTime = ":0" + strtime + FTime;
    }

    strtime = parseInt(parseInt(TimeSeconds/60)/60) % 60;
    if(strtime >= 10)
    {
        FTime = strtime + FTime;
    }
    else
    {
        FTime = "0" + strtime + FTime;
    }
    return(FTime);
}

//获取带中文的字符串长度
String.prototype.getBytes = function() 
{    
    var cArr = this.match(/[^\x00-\xff]/ig);   
    return this.length + (cArr == null ? 0 : cArr.length*2);    
}

/*====================================================================
    RGB颜色转为十六进制颜色
====================================================================*/
function RGB2HEX(rgb)
{
    rgb = rgb.match(/^rgb\((\d+),\s*(\d+),\s*(\d+)\)$/);
    return "#" + hex(rgb[1]) + hex(rgb[2]) + hex(rgb[3]);
}

function hex(x)
{
    return ("0" + parseInt(x).toString(16)).slice(-2);
}

var g_is_msie = false;
//判断浏览器是否是IE
function isMSIE(){
    var u_agent  = navigator.userAgent.toLowerCase();
    g_is_msie = /msie/.test(u_agent) || (u_agent.indexOf('trident')>-1&&u_agent.indexOf('rv:11')>-1);
}

/*===================================================================
    VENC_SIZE_E and string transform
===================================================================*/
function VencStr2Size(VencStr)
{
    VencStr = VencStr.toUpperCase();
    if ("QCIF" == VencStr)
    {
        return 0;
    }
    else if ("CIF" == VencStr)
    {
        return 1;
    }
    else if ("D1" == VencStr)
    {
        return 2;
    }
    else if ("720P" == VencStr)
    {
        return 3;
    }
    else if ("UVGA" == VencStr)
    {
        return 4;
    }
    else if ("1080P" == VencStr)
    {
        return 5;
    }
    
    return -1;
}

function VencSize2Str(VencSize)
{
    var ptr = "";
    
    switch (parseInt(VencSize))
    {
        case 0:
        {
            ptr = "QCIF";
            break;
        }

        case 1:
        {
            ptr = "CIF";
            break;
        }

        case 2:
        {
            ptr = "D1";
            break;
        }

        case 3:
        {
            ptr = "720P";
            break;
        }

        case 4:
        {
            ptr = "UVGA";
            break;
        }
        
        case 5:
        {
            ptr = "1080P";
            break;
        }

        default:
        {
            ptr = "UNKNOWN";
            break;
        }
    }

    return ptr;
}

//执行语言环境
Language_sel();
isMSIE();

//响应onkeypress事件, 这个事件只包括英文字母、数字等一系列的符号, 并不包括汉字
function IsDigit(){
    //event = window.event || arguments.callee.caller.arguments[0];
    if ((event.keyCode != 46) && (event.keyCode != 13)){
        return ((event.keyCode >= 48) && (event.keyCode <= 57));
    }else if (event.keyCode == 46){
        return ((event.keyCode >= 48) && (event.keyCode <= 57));
    }else{
        return  (event.keyCode);
    }
}

//响应onkeyup事件, 只能输入整数
function IsDigitUp(){
    var arg0 = arguments[0];
    
    var NumberRegExp = new RegExp("^\\d+$","g");    
    var myRegExp = new RegExp("[^0-9]+");       //匹配不在0-9之内的数
    if (NumberRegExp.test(arg0.value) == false){
        arg0.value = arg0.value.replace(myRegExp, '');
    }
}

function IsDigitBlur(){
    var arg0 = arguments[0];
    if(arg0.value==''){
        arg0.value = '00';
    }
}

//响应onkeyup事件, 限制用户输入以下特殊字符
function IsInputUp(){
    var arg0 = arguments[0];
    var myRegExp = /["'<>%;)(&+“”\[\]!！？?\\#￥$^；【】。]/;
    if(myRegExp.test(arg0.value)){
          arg0.value = arg0.value.replace(myRegExp, '');
    }
}

//字符串第一个字母变大写
function transferStrFirstCharToUpper(str){
    if(null == str || '' == str)
        return str;
    str = (str.charAt(0)).toUpperCase()+str.substring(1);
    return str;
}

var selfName = self.location.pathname;
//清空cookie后回到登陆页面
var strUrl = GetCookieByKey("url");
if (-1 == strUrl)
{
    var __target_link = "/login.asp";
    if ("/login.asp" != selfName && "login.asp" != selfName)
    {
        parent.location = __target_link;
    }
}

//页面F5刷新时获取焦点
$(function(){
    $(document).keydown(function(event){
       if(event.keyCode==116){
            window.focus();
       }
    });
});