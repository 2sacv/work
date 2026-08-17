var homeFrame;//页面Iframe
var curr_menu_id;//当前选中菜单ID
var firstMenu = 0;//第一次点击菜单
$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf || typeof(lf) =='undefined' || lf === "null" || 0 > parseInt(lf)){
          parent.location.href = "/login.asp";
    }else{
        homeFrame = window.parent.document.getElementById("homeFrame");
        if($.cookie("graintype") == 0){
          $("#menu_video_capture").hide();
        }
        getPlatform();
        judgeMotionFollow();
        _init_load();
    }
});

judgeMotionFollow = function(){
    GetJCP({cmd: "showweb -act list", ParseJCP: function(jcpGet){
        if(jcpGet !== "Error"){
            var en_follow = typeof(jcpGet.follow) == 'undefined'?0:jcpGet.follow;
            Set_cookie("en_follow",en_follow);
        }
    }});
}

getPlatform = function(){
    GetJCP({cmd: "version -act list", ParseJCP: function(jcpGet){
        if(jcpGet !== "Error"){
            var platform = typeof(jcpGet.platform)== 'undefined'?"normal":jcpGet.platform;
            platform = platform.toLowerCase();
            if(platform.charAt(platform.length-1)===','){
                platform = platform.substring(0,platform.length-1);
            }

            var arr = [];

            //显示tencent和guobiao，需要使用小写
            if(platform.indexOf('gbptp') >= 0) {
                arr.push('cloudplatform');
                arr.push('guobiao');
            } else if (platform.indexOf('tencent') >= 0) {
                arr.push('cloudplatform');
            } else if (platform.indexOf('gb') >= 0) {
                arr.push('guobiao');
            } else {
            
                var arrPF = platform.split(",");
                //如果平台没有包含"guobiao","hxht","hngs","tslive","p2p","jstar","tencent","wstk"中的至少一个,屏蔽平台设置菜单
                
                for(var i=0,l=arrPF.length;i<l;i++){
                    //**P2P显示腾讯云P2P
                    if(arrPF[i].indexOf('p2p') > 0){
                        arrPF[i] = "cloudplatform";
                    }
                    if(g_platform.indexOf([arrPF[i]]) != -1){
                        arr.push(arrPF[i]);
                    }
                }
            }
            
            if(arr.length === 0){
                $("#menu_platform_set").hide();
                if(GetCookieByKey("curr_menu_id") === 'menu_platform_set'){
                    Set_cookie("curr_menu_id","menu_sys_set");
                }
            }else{
                Set_cookie("platform",arr.join(","));
            } 
            
        }else{
            $("#menu_platform_set").hide();
        }
    }});
}

jump_page = function(title,path){
    homeFrame.src = path;
    window.parent.document.title = title;
}

_init_load = function(){
    $("#left_btn button").click(function(){
        var menuId = $(this).attr("id");
        if(menuId != curr_menu_id || firstMenu == 0){
            if(curr_menu_id == -1){
                $(this).addClass("left_btn");
            }else{
                $("#"+curr_menu_id).removeClass("left_btn")
                $(this).addClass("left_btn");
            }
            curr_menu_id = menuId;
            Set_cookie("curr_menu_id",curr_menu_id);

            switch(menuId){
                case "menu_sys_set":jump_page(IDC_MENU_SYSTEM_SET,'sysinfo.asp');break;
                case "menu_net_set":jump_page(IDC_MENU_NETWORKSET,'networksetting.asp');break;
                case "menu_audio_video":jump_page(IDC_MENU_AUDIOVIDEO,'audiovideo.asp');break;
                case "menu_video_capture":jump_page(IDC_MENU_VIDEOCAPTURE,'videosnap.asp');break;
                case "menu_alarm_set":jump_page(IDC_ALARM_SET,'alarmsetting.asp');break;
                case "menu_local_set":jump_page(IDC_MENU_LOCALSET,'localsetting.asp');break;
                case "menu_platform_set":jump_page(IDC_MENU_PLATFORMSET,'platformsetting.asp');break;
                default:jump_page(IDC_MENU_SYSTEM_SET,'sysinfo.asp');break;
           }
        }
        firstMenu++;
        window.focus();
    });

    curr_menu_id = GetCookieByKey("curr_menu_id");

    if(curr_menu_id == -1){
        $("#menu_sys_set").click();
    }else{
        $("#"+curr_menu_id).click();
    }
}
