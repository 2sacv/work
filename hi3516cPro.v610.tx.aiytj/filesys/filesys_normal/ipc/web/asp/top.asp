<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta http-equiv="pragma" content="no-cache" />
<meta http-equiv="Cache-Control" content="no-cache, must-revalidate" />
<meta http-equiv="expires" content="Thu, 1 Jan 1970 00:00:01 GMT" />

<link href="/css/bootstrap.css" type="text/css" rel="stylesheet"/>
<link href="/css/index.css" type="text/css" rel="stylesheet"/>
<script type="text/javascript" src="/jquery/jquery-1.11.1-min.js"></script>
<script type="text/javascript" src="/jquery/jquery.cookie.js"></script>
<script type="text/javascript" src="/js/public_function.js"></script>
<script type="text/javascript" src="/js/jcpcmd.js"></script>
<script type="text/javascript">
    function clickPlay(){
        Set_cookie("curr_menu_id",-1);
        parent.location.href ='mainview.asp?g=1.0';
    }

    function clickLogout(){
        if(confirm(IDC_MSGBOX_MSG)){
           Set_cookie("curr_menu_id",-1);
           deleteCookie("loginflag_"+g_hostname);
           parent.location.href ='../login.asp?g=1.0';
        }else{
          window.focus();
        }
    }

    function clickPlayback(){
        parent.location.href ='playback.asp?g=1.0';
    }

    function showLogo(){
      var $logo = $("#top_logo_image");
      $logo.attr("src","/image/logo.png");
      $logo.show();
      GetJCP({cmd: "version -act list",ParseJCP: function(result){
         if(result != 'Error'){
           $("#top_title_span").html(result.devtype); //tag_devtype
         }
      }});
    }

    $(function(){
        if($.cookie("graintype") == 0){
          $("#btn_play_back").hide();
        }
        showLogo();
    });
</script>
<style type="text/css">
    #top_page_div{height: 60px; float:right;}
    #top_page_div button{ 
      width:130px; margin-top:5px;margin-right:0px;
    }
    .index_btn{
        width:145px; 
        height:50px;font-size: 18px;text-align: center;
        background: url("../image/bg_tab_btn_bottom2.png");
    }
    .divBody{
        height:65px;
    }
</style>
</head>

<body style="background:#141414">
   <div class="divBody">
     <div id='top_logo_div'>
        <img id="top_logo_image" src="/image/logo.png" href="#" style="display:none;margin-top:-8px;"/>
        <span id="top_title_span" class="logo_title"></span>
     </div>
     <div id="top_page_div">
        <button class='btn btn-inverse btn-black index_btn' onclick="clickPlay();" style="margin-right:0px;">
            <img src="/image/playvideo.png" style="margin-top:-3px;">
            <span><script>dwn(IDC_PLAYVIDEO)</script></span>
        </button>
        <button class='btn btn-inverse btn-black index_btn' onclick="clickPlayback()"  style="margin-left:0px;margin-right:0px;padding-left:0px;" id="btn_play_back">
            <img src="/image/playback.png" style="margin-top:-3px;">
            <span><script>dwn(IDC_PLAYBACK)</script></span>
        </button>
        <button class='btn btn-inverse btn-black index_btn' onclick="clickLogout();" style="margin-left:0px;">
            <img src="/image/logout.png" style="margin-top:-3px;">
            <span><script>dwn(IDC_EXIT)</script></span>
        </button>
    </div> 
   </div>
</body>
</html>
