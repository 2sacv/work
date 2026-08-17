$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf || typeof(lf) =='undefined' || 0 > parseInt(lf))
    {
          parent.location.href = "/login.asp";
    }else{
        if($.cookie("graintype") == 0){
          $("#ftp_download").hide();
        }
        $("#frontend_tabs").tabs()
        $(".sTime").selectTime()
        _init_dev_opt();
        _init_language()
        _init_player()
        _get_devrecordcfg()
        //_init_videosize()
        _init_snapsize()
        _init_video_click()
        $("select").bind("change",function(){
             window.focus();
        }); 
        //解决非IE下点击前端抓拍视频出不来问题
        if(!g_is_msie && self.location.toString().indexOf("page=snap") >0){
            $("#snap_lab").click();
        }
    }
})

var hide_sdinfo = 0;
var disk,video_status_times = 0;
var g_video_flag = 0;
record_status_fresh = function(){
    $.ajax({type: "GET", 
            url: "?jcpcmd=devrecordcfg -act list", 
            dataType: "script", 
            cache: false,
            timeout: 1900,
            success:function(result){
                result = $.trim(result);
                if (result.indexOf('[Success]')>0) {
                   var szResult = result.split("[Success]")[1];
                   showVideoStatus(szResult);
                }
            },
            error:function(XMLHttpRequest, textStatus, errorThrown){
            }
    });
}

function _init_dev_opt() {
    GetJCP({cmd: "devappopt -act list", ParseJCP: function(info){
        if(info!='Error'){
            if (info.sdinfo != undefined) {
                if (info.sdinfo == 1) {
                    hide_sdinfo = 1;
                    $('#tbDisk tr').find('td:eq(1)').hide();
                    $('#tbDisk tr').find('td:eq(2)').hide();
                    $('#tbDisk tr').find('td:eq(3)').hide();
                }
            }
        }
    }})
}

record_status = function(){
    record_status_fresh();
    video_status_times = setInterval('record_status_fresh()',2000);
}

function showVideoStatus(str){
    if(g_video_flag == 1){ 
        g_video_flag = 0;
    }else{
        disk = GetRtspKeyStr(str,"diskname0");

        if (typeof(disk) == 'undefined'){
            $("#video_status_lab").html(IDC_STOP_VIDEO);
            window.focus();
            return;
        }else{
            var s = GetRtspKeyStr(str,"recordstatus0");
            if(0 == parseInt(s))
            {
                $("#video_status_lab").html(IDC_STOP_VIDEO);
                $("#start_video").html(IDC_MANUAL_VIDEO);
            }
            else if(1 == parseInt(s))
            {
                $("#video_status_lab").html(IDC_STATUS_MANUAL);
                $("#start_video").html(IDC_STOP_MANUAL_VIDEO);
            }
            else if(2 == parseInt(s))
            {
                $("#video_status_lab").html(IDC_STATUS_ALARM);
                $("#start_video").html(IDC_MANUAL_VIDEO);
            }
            else if(3 == parseInt(s))
            {
                $("#video_status_lab").html(IDC_STATUS_ALARM);
                $("#start_video").html(IDC_MANUAL_VIDEO);
            }
            else if(4 == parseInt(s))
            {
                $("#video_status_lab").html(IDC_STATUS_TASK);
                $("#start_video").html(IDC_MANUAL_VIDEO);
            }
        }
    }
}

_get_devrecordcfg = function(){
    GetJCP({cmd: "devrecordcfg -act list", ParseJCP: function(info){
        if(info!='Error'){
            $('#video_time').selectTime('setData',info.schedule)
            $("#pre_time").val(info.prerecordtime)
            $("#file_length").val(info.filetime)
            $("#alarm_length").val(info.recordtime)
            if(1 == info.recordstatus0){
                $("#start_video").html(IDC_STOP_VIDEO);
            }
            $("input[name='disk_strategy'][value='"+info.diskstrategy+"']").attr("checked",true);
            $("input[name='pre_rdo'][value='" + parseInt(info.prerecorden) + "']").prop("checked",true);
            $("input[name='rec_type'][value=" + parseInt(info.rec_type) + "]").prop("checked",true);
            _pretime_disabled();
            record_status();
            _disk();
        }else{
            _get_devrecordcfg();
        }
    }})
}

_get_capturecfg = function(){
    GetJCP({cmd: "capturecfg -act list", ParseJCP: function(info){
        if(info!='Error'){
            $("#snap_num").val(info.alarmnum);
            $("#stream_size").val(info.vesize);
            $("#time_interval").val(info.interv);
            $("#alarm_time_interval").val(info.alarminterv);
            if(1 == parseInt(info.alarmnum)){
                $("#alarm_time_interval").attr("disabled",true);
            }
            $("input[name='snap_stream'][value='" + info.vesize + "']").attr("checked",true);
            $("#snap_time").selectTime("setData",info.timestrategy)
        }
    }})
}

_get_remote = function(){
    GetJCP({cmd: "nfsrecordcfg -act list", ParseJCP: function(info){
        if(info!='Error'){
            $("input[name='nfs_switch'][value='"+info.enable+"']").attr("checked",true);
            if(info.enable == '0'){
                _disabled('nfs')
            }
            $("#nfs_path").attr("value",info.path);
            $("#nfs_user").attr("value",info.user);
            $("#nfs_pwd").attr("value",info.passwd);
        }
    }})

    GetJCP({cmd: "sambarecordcfg -act list", ParseJCP: function(info){
        if(info!='Error'){
            $("input[name='samba_switch'][value='"+info.enable+"']").attr("checked",true);
            if(info.enable == '0'){
                _disabled('samba')
            }
            $("#samba_path").attr("value",info.path);
            $("#samba_user").attr("value",info.user);
            $("#samba_pwd").attr("value",info.passwd);
        }
    }})
}

_init_videosize = function(){
    var m = $.cookie("master_stream");
    var s = $.cookie("slave_stream");
    var t = "";
    if(1 == parseInt($.cookie("master_enb")))
    {
        t += typeof(m) == 'undefined' ? "" : "<input type='radio' id='video_m' name='video_stream' value='" + m + "' checked style='margin-left: 15px;><label for='video_m'>" + stream[m] + "</label>";
    }

    if(1 == parseInt($.cookie("slave_enb")))
    {
        t += typeof(s) == 'undefined' ? "" : "<input type='radio' id='video_s' name='video_stream' value='" + s + "' style='margin-left: 10px;'><label for='video_s'>" + stream[s] + "</label>";    
    }
    $("#stream_td").append(t)
}

_init_snapsize = function(){
        var m = $.cookie("master_stream");
        var s = $.cookie("slave_stream");
        var t = "";
        if(1 == parseInt($.cookie("master_enb")))
        {
            t += typeof(m) == 'undefined' ? "" : "<input type='radio' name='snap_stream' id='snap_m' value='" + m + "' checked><label for='snap_m'>" + stream[m]+ "</label>";
        }

        if(1 == parseInt($.cookie("slave_enb")))
        {
            t += typeof(s) == 'undefined' ? "" : "<input type='radio' name='snap_stream' id='snap_s' value='" + s + "' style='margin-left: 10px;'><label for='snap_s'>" + stream[s]+ "</label>";
        }
        $("#stream_size").append(t)
}


var g_disk_freshing = false;
_disk = function(){ 
    if(g_disk_freshing){
        return;
    }
    g_disk_freshing = true;
    $("#diskRefreshLoad").show();
    $("#tbDisk tr:not(:first)").remove();
    var diskname, disktotal, diskusage, diskfree,insert;
    GetJCP({cmd: "devrecordcfg -act list", ParseJCP: function(info){
        if(info!='Error'){
            var noDisk = 0;
            for (var i = 0; i < 16; i++)
            {
                diskname = info['diskname'+i];
                if (typeof(diskname) == 'undefined'){ 
                    noDisk++;
                    continue; 
                }

                disktotal = info['disktotal'+i]
                diskfree = info['diskfree'+i]
                diskusage = disktotal - diskfree;
                insert = '<tr height="30" align="center">'
                insert += '<td valign="middle" id="sysName">'+diskname+'</td>';//分区
                insert += '<td valign="middle">'+disktotal+'MB</td>';//总量
                insert += '<td valign="middle">'+diskusage+'MB</td>';//已用
                insert += '<td valign="middle">'+diskfree+'MB</td>';//剩余
                disktotal = disktotal >= 1 ? disktotal : 1;
                var haveuseds = (diskusage * 100 / disktotal).toFixed(2);
                insert += '<td valign="middle" >'+ haveuseds +'</td>';//已用%
                Set_cookie("haveuseds",haveuseds);
                insert += '<td valign="middle" id="tbProgress">';//格式化
                if (diskname != "samba" && diskname != "nfs")
                {
                    insert += '<button class="btn btn-inverse btn-black" onclick=Format(\"' + diskname + '\");>' + IDC_FORMAT + '</button>';
                }   
                else
                {
                    insert += '&nbsp';
                }
                insert += '</td></tr>';
                $("#tbDisk").append(insert);
            }
            if(noDisk == 16){
                insert = '<tr height="30" align="center"><td colspan="6">'+IDC_DISK_INFO_NO+'</td></tr>';
                $("#tbDisk").append(insert);
            }
            $("#diskRefreshLoad").hide();
        }else{
            $("#tbDisk").append('<tr height="30" align="center"><td colspan="6">'+IDC_DISK_INFO_NO+'</td></tr>');
            $("#diskRefreshLoad").hide();
        }
        
        if (hide_sdinfo == 1) {
            $('#tbDisk tr').find('td:eq(1)').hide();
            $('#tbDisk tr').find('td:eq(2)').hide();
            $('#tbDisk tr').find('td:eq(3)').hide();
        }
        g_disk_freshing = false;
     }});

}

var g_curr_tab = "video_lab";
show = function(index){
    if((index == 0 &&  g_curr_tab == "video_lab") || (index == 1 &&  g_curr_tab == "snap_lab")){
        return;
    }
   
    window.clearTimeout(flushTimer);
    if(index == 1){
        if(g_is_msie || g_curr_tab == "video_lab"){
            g_curr_tab = "snap_lab";
            $("#video_tab").hide();
            $("#snap_tab").show();
            window.clearInterval(video_status_times);
            _get_capturecfg();
        }else{
           window.location.href = window.location.href.split("?")[0] + "?page=snap";
        }
    }
    else{
        if(g_is_msie || g_curr_tab == "snap_lab"){
            g_curr_tab = "video_lab";
            $("#snap_tab").hide();
            $("#video_tab").show();
            record_status();
        }else{
            window.location.href = window.location.href.split("?")[0];
        }
    }
}

Format = function(name){
    GetJCP({cmd: "devrecordcfg -act list", ParseJCP: function(info){
        if(info!='Error'){
            if(typeof info.recordstatus0 != 'undefined' && info.recordstatus0>0){
                alert(IDC_CANNOT_GSH);
            }else {
                var diskname = name;
                var temp,ret;
                if(confirm(IDC_MSGBOX_FORMAT))
                {
                    alert(IDC_GSH_CLOSES);
                    
                    $("#tbDisk tr").each(function(i){
                        if ($(this).children().eq(0).text() == diskname)
                        {
                            // 利用javascript的闭包特性, 使得后续的函数可以访问这2个变量, 包括前面的diskname和temp
                            var idName = "progress" + diskname;
                            var thisObj = $(this).children().eq(5);

                            thisObj.html('<div id = "' + idName + '"' + 'style="height:15px;width:102px;"></div>');

                            jcpstrs = "format -name " + diskname + " -enable 1";
                            GetJCP({cmd: jcpstrs});

                            // 根据名称生成进度条
                            $("#" + idName).progressbar({
                                value: 0
                            });

                            // 定时执行函数
                            $(document).everyTime(3000, "progress" + diskname, function (){

                                GetJCP({cmd:"mkdosfsprogbar -act list -progbar "+diskname,ParseJCP:function(jcpobj)
                                {
                                    ret = jcpobj.progbar;

                               
                                    if(ret == 100)
                                    {
                                        // 停止定时器
                                        $(document).stopTime(idName);
                                        $("#" + idName).progressbar("option","value",100);
                                        alert(IDC_GSH_FULFILL);
                                        _disk();
                                    }
                                    else if(ret == -1)
                                    {
                                        alert(IDC_GSH);
                                        $(document).stopTime(idName);
                                        thisObj.html('<button class="btn btn-inverse btn-black" onclick=Format(\"' + diskname + '\");>' + IDC_FORMAT + '</button>');
                                    }else{
                                         $("#" + idName).progressbar("option","value",parseInt(ret));
                                    }
                                }});
                            });
                        }
                    });
                }
           }
        }
    }});
}

_init_player = function(){
    if(g_is_msie){
      $("#video_ipcamer").html('<object id="IPCamera" name="IPCamera" CLASSID="CLSID:2319F6E6-ABD3-4b68-BADF-05D8796FA072" width="500" height="370"></object>');
    }else{
      $("#video_ipcamer").html('<object id="IPCamera" name="IPCamera" type="application/npipcam" width="500" height="370"></object>');
    }
    document.IPCamera.IPCSetWindowMode(1);
    var type = GetCookieByKey("ljtypes");
    type = type== -1?1:type;
    document.IPCamera.IPCStartPreviewEx(0, GetCookieByKey("url"), 0, parseInt(type), GetCookieByKey("rtspport"),GetCookieByKey("user"), GetCookieByKey("passwd"), GetCookieByKey("stream"),"V2.00");
}

var flushTimer = 0;
refreshs = function(){
    if($("#auto_refresh").is(':checked'))
    {
        _disk()
        flushTimer = setTimeout('refreshs()', 1000 * parseInt($("#disk_intervals").val()));
    }
    else
        window.clearTimeout(flushTimer);
}

JcpSet = function(str,prompt){
        GetJCP({cmd: str,ParseJCP: function(result){
            if(result != "Error"){
                alert(prompt)
                window.focus();
            }
        }})
}

remote = function(){
    g_curr_tab = "remote_lab";
    window.clearTimeout(flushTimer);
    window.clearInterval(video_status_times);
    _get_remote()
}

_pretime_disabled = function(){
    var enb = $("input[name='pre_rdo']:checked").val()
    if(1 == enb){
        $("#pre_time").prop("disabled",false);
    }
    else
        $("#pre_time").prop("disabled",true);
}

_init_video_click = function(){
    $("input[name='pre_rdo']").click(function(){
        _pretime_disabled()
    })

    $("#refresh").click(function(){
        _disk()
        window.focus();
    })

    $("#auto_refresh").click(function(){
        refreshs()
        window.focus();
    })

    $("#snap_picture").click(function()
    { 
        GetJCP({cmd: "devrecordcfg -act list",ParseJCP: function(result){
            if(result != "Error"){
                var diskname = result['diskname0'];
                if (typeof(diskname) == 'undefined'){
                    alert(IDC_HDR_NONE+IDC_DISK_STORAGE)
                    window.focus();
                    return;
                }else if(95 < $.cookie("haveuseds")){
                    alert(IDC_DISK_FULL);
                    window.focus();
                    return;
                }else{
                    JcpSet("capturecfg -act manual",IDC_SNAP+IDC_SUCCESS);  
                    window.focus();  
                }
            }else{
                alert(IDC_HDR_NONE+IDC_DISK_STORAGE);
                window.focus();
            }
        }});
    });

    $("#snap_save").click(function(){
        var time_interval = $("#time_interval").val();
        var alarm_interval = $("#alarm_time_interval").val() ;
        if( time_interval > 600 || time_interval < 1){
                alert(IDC_TIMING+IDC_TIME_INTERVAL+IDC_VIDEOSIZE_PROMPT);
                return;
        }
        if(alarm_interval > 10 || alarm_interval < 1){
                alert(IDC_ALARM+IDC_TIME_INTERVAL+IDC_VIDEOSIZE_PROMPT);
                return;
        }

        var str = "capturecfg -act set  -interv " + time_interval;
        str += " -alarmnum " + $("#snap_num").val() + " -alarminterv " + alarm_interval;
        str += " -timestrategy " + $('#snap_time').selectTime('getData');
        str += " -vesize " + $("input[name='snap_stream']:checked").val();
        JcpSet(str,IDC_SAVE+IDC_SUCCESS);
    })

    $("#start_video").click(function(){
        GetJCP({cmd: "devrecordcfg -act list",ParseJCP: function(result){
            if(result != "Error"){
                var diskname = result['diskname0'];
                if (typeof(diskname) == 'undefined'){
                    alert(IDC_HDR_NONE+IDC_DISK_STORAGE)
                    window.focus();
                    return;
                }else{
                    var s = parseInt(result["recordstatus0"]);
                    if(0 == s) //没有录像
                    {
                        g_video_flag = 1;
                        GetJCP({cmd:"devrecordcfg -act set -recorden 1"});
                        $("#video_status_lab").html(IDC_STATUS_MANUAL);
                        $("#start_video").html(IDC_STOP_MANUAL_VIDEO);
                    }
                    else if(1 == s) //手动录像
                    {
                        g_video_flag = 1;
                        GetJCP({cmd:"devrecordcfg -act set -recorden 0"});
                        $("#video_status_lab").html(IDC_STOP_VIDEO);
                        $("#start_video").html(IDC_MANUAL_VIDEO);
                    }
                    else if(2 == s || 3 == s) //报警录像
                    {
                        alert(IDC_STATUS_ALARM+","+IDC_MANUAL_VIDEO_FAIL);
                    }
                    else if(4 == s) //计划任务录像
                    {
                        alert(IDC_STATUS_TASK+","+IDC_MANUAL_VIDEO_FAIL);
                    }
                    window.focus();

                }
                
            }
        }});
    });

    $("#http_download").click(function(){
        var w = ":"+$.cookie("webport")+"/mnt";
        window.open("http://"+ document.domain + w)
        window.focus();
    });

    $("#ftp_download").click(function(){
        window.open("ftp://" + document.domain+ ":" + $.cookie("ftport"));
        window.focus();
    });

    $("#video_save").click(function(){
        var pre_time = $("#pre_time").val();
        var alarm_length = $("#alarm_length").val() ;
        var file_length = $("#file_length").val();
        if(alarm_length > 90 || alarm_length < 10){
            alert(IDC_ALARM_LENGTH+IDC_VIDEOSIZE_PROMPT);
            window.focus();
            return;
        }

        if(file_length  > 15 || file_length <= 0){
            alert(IDC_FILE_LENGTH+IDC_VIDEOSIZE_PROMPT)
            window.focus();
            return;
        }

        if( pre_time < 5 || pre_time > 10){
            alert(IDC_PRESET+IDC_TIME+IDC_VIDEOSIZE_PROMPT);
            window.focus();
            return;   
        }

        var jcpstr = "devrecordcfg -act set -schedule " + $(".sTime").selectTime('getData')
        jcpstr += " -filetime " + file_length + " -recordtime " + alarm_length + " -prerecordtime " + pre_time;
        jcpstr += " -rec_type " + $("input[name='rec_type']:checked").val();
        jcpstr += " -diskstrategy " + $("input[name='disk_strategy']:checked").val();
        jcpstr += " -prerecorden " + $("input[name='pre_rdo']:checked").val()
        JcpSet(jcpstr,IDC_SAVE + IDC_SUCCESS)
        window.focus();
    })

    $("#nfs_save").click(function(){
        var path = $("#nfs_path").val();
        var cArr = path.match(/[^\x00-\xff]/ig);   
        var len =  path.length + (cArr == null ? 0 : cArr.length*2);  
        if(len>127){
            alert(IDC_REMOTE_PATH_MSG_RANGE);
            window.focus();
            return;
        }
        /*if(path.length>2 && path.lastIndexOf("//")==(path.length-2)){
            alert(IDC_REMOTE_PATH_ERROR+IDC_FOREXAMPLE+"192.168.1.217:/tmp");
            return;
        }*/
        if(path.length>2 && path.lastIndexOf("//")>0){
            alert(IDC_REMOTE_PATH_ERROR+IDC_FOREXAMPLE+"192.168.1.217:/tmp");
            window.focus();
            return;
        }
        var n = $("input[name='nfs_switch']:checked").val();
        var jcpstr = "nfsrecordcfg -act set -enable " + n + " -path " + $("#nfs_path").val();
        jcpstr += " -user " + $("#nfs_user").val() + " -passwd " + $("#nfs_pwd").val();
        GetJCP({cmd: jcpstr,ParseJCP: function(result){
            if(result == "Error"){
                alert(IDC_REMOTE_PATH_ERROR+IDC_FOREXAMPLE+"192.168.1.217:/tmp");
                window.focus();
            }
            else{
                alert(IDC_SAVE + IDC_SUCCESS)
                window.focus();
            }
                
        }});
    })

    $("#samba_save").click(function(){
        var user = $("#samba_user").val();
        var pwd = $("#samba_pwd").val();
        var path = $("#samba_path").val();
        if(user == '' || user.length==0){
            alert(IDC_USERNAME_NOEMPTY);
            window.focus();
            return;
        }
        if(pwd == '' || pwd.length==0){
            alert(IDC_PASSWORD_NOEMPTY);
            window.focus();
            return;
        }
        var cArr = path.match(/[^\x00-\xff]/ig);   
        var len =  path.length + (cArr == null ? 0 : cArr.length*2);  
        if(len>127){
            alert(IDC_REMOTE_PATH_MSG_RANGE);
            window.focus();
            return;
        }
        if(path.length>2 && path.lastIndexOf("//")>0){
            alert(IDC_REMOTE_PATH_ERROR+IDC_FOREXAMPLE+"//192.168.1.217/tmp");
            window.focus();
            return;
        }
        var n = $("input[name='samba_switch']:checked").val();
        var jcpstr = "sambarecordcfg -act set -enable " + n + " -path " + path + " -user " + user + " -passwd " + pwd;
        GetJCP({cmd: jcpstr,ParseJCP: function(result){
            if(result == "Error"){
                alert(IDC_REMOTE_PATH_ERROR+IDC_FOREXAMPLE+"//192.168.1.217/tmp");
                window.focus();
            }
            else{
                alert(IDC_SAVE + IDC_SUCCESS)
                window.focus();
            }

        }});
    })

    $("input[name='nfs_switch']").click(function(){
        if(0 == $("input[name='nfs_switch']:checked").val())
        {
            _disabled('nfs')
        }
        else
            _enabled('nfs')
    })

    $("input[name='samba_switch']").click(function(){
        if(0 == $("input[name='samba_switch']:checked").val())
        {
            _disabled('samba')
        }
        else
            _enabled('samba')
    })
}

disk_tab = function(){
    g_curr_tab = "disk_lab";
    window.clearInterval(video_status_times);
    if($("#auto_refresh").prop("checked"))
    {
        refreshs()
    }
}

_enabled = function(str){
    if('nfs' == str){
        $("#nfs_path").attr("disabled",false)
        $("#nfs_user").attr("disabled",false)
        $("#nfs_pwd").attr("disabled",false)
    }
    else
    {
        $("#samba_path").attr("disabled",false)
        $("#samba_user").attr("disabled",false)
        $("#samba_pwd").attr("disabled",false)
    }
}

_disabled = function(str){
    if('nfs' == str){
        $("#nfs_path").attr("disabled",true)
        $("#nfs_user").attr("disabled",true)
        $("#nfs_pwd").attr("disabled",true)
    }
    else
    {
        $("#samba_path").attr("disabled",true)
        $("#samba_user").attr("disabled",true)
        $("#samba_pwd").attr("disabled",true)
    }
}

_init_language = function(){
    $("#video_lab").html(IDC_FRONTEND + IDC_VIDEO)
    $("#snap_lab").html(IDC_FRONTEND + IDC_SNAP)
    $("#remote_lab").html(IDC_REMOTE_RECORD)
    $("#disk_lab").html(IDC_DISK_STATUS)
    $("#pre_time_lab").html(IDC_PRESET+IDC_TIME+": ")
    $("#pre_time_prompt").html("(5-10) "+IDC_SECONDS)
    $("#file_length_span").html(IDC_FILE_LENGTH+": ")
    $("#file_prompt").html("(1~15) "+IDC_MINUTES)
    $("#alarm_length_span").html(IDC_ALARM_LENGTH+": ")
    $("#alarm_prompt").html(" (10~90) " + IDC_SECONDS)
    $("#disk_space_strategy").html(IDC_DISK_STRATEGY)
    $("#stop_video_lab").html(IDC_STOP_VIDEO)
    $("#del_oldfile").html(IDC_DEL_OLDFILE)
    $("#video_size").html(IDC_VIDEO_SIZE)
    $("#pre_enb").html(IDC_PRESET+" :")
    $("#pre_open_lab").html(IDC_OPEN)
    $("#pre_close_lab").html(IDC_CLOSE)
    $("#video_status").html(IDC_VIDEO+IDC_STATUS+" :   ")

    $("#video_save").html(IDC_SAVE);
    $("#start_video").html(IDC_MANUAL_VIDEO);
    $("#http_download").html(IDC_HTTP_DOWNLOAD);
    $("#ftp_download").html(IDC_FTP_DOWNLOAD);
    $("#open_pre").html(IDC_OPEN+IDC_PRESET);
    $("#video_time_strategy,#snap_time_strategy").html(IDC_TIME_PROTECTION);

    $("#snap_num_lab").html(IDC_ALARM+IDC_SNAP_NUM+" :   ");
    $("#stream_size_lab").html(IDC_SNAP_SIZE+" :   ");
    $("#interval_lab").html(IDC_TIMING+IDC_TIME_INTERVAL+" :   ");
    $("#disk_interval").html(IDC_FLUSH_INTERVAL);
    $("#second").html("(1~600)"+IDC_SECONDS);
    $("#alarm_second").html("(1~10)"+IDC_SECONDS);
    $("#snap_picture").html(IDC_SNAP_IMG);
    $("#alarm_interval_lab").html(IDC_ALARM+IDC_TIME_INTERVAL+" :   ")

    $("#remote_set_lab").html(IDC_REMOTE+IDC_SET+" :   ");
    $("#nfs_enb_lab,#samba_enb_lab").html(IDC_REMOTE+IDC_SWITCH+" :   ");
    $("#nfs_open,#samba_open").html(IDC_ENABLE)
    $("#nfs_close,#samba_close").html(IDC_UNALBE)
    $("#nfs_path_lab,#samba_path_lab").html(IDC_REMOTE+IDC_PATH+" :   ");
    $("#nfs_user_lab,#samba_user_lab").html(IDC_USER+" :   ");
    $("#nfs_pwd_lab,#samba_pwd_lab").html(IDC_PWD+" :   ");
    $("#nfs_save,#samba_save,#snap_save").html(IDC_SAVE)
    $("#refresh_lab").html(IDC_PTZ_AUTO+IDC_REFRESH);
    $("#refresh").html(IDC_REFRESH)

    $("#partition").html(IDC_PARTITION)
    $("#total").html(IDC_TOTAL)
    $("#haveused").html(IDC_HAVE_USED)
    $("#remaining").html(IDC_REMAINING)
    $("#haveuseds").html(IDC_HAVE_USED+"%")
    $("#format").html(IDC_FORMAT)
    $("#formatTip").html(IDC_GSH_TIP);

    var o = "";
    for(var i = 1; i <= 3;i ++){
        o += "<option value='" + i + "'>" + i + "</option>"
    }
    $("#snap_num").append(o);
    $("#snap_num").change(function(){
        if(1 == $(this).val()){
            $("#alarm_time_interval").attr("disabled",true);
        }else{
            $("#alarm_time_interval").attr("disabled",false);
        }
    });

    var t = ""
    for(var i = 1; i < 13; i ++){
        t += "<option value='" + i * 5 + "'>" + i * 5 + "</option>"
    }
    $("#disk_intervals").append(t)
}


var $w = $(window).width();
if($w < 1200){
   $("body").css("width",1200);
}
$(window).resize(function(){
    var $w = $(window).width();
    if($w < 1200){
       $("body").css("width",1200);
    }else{
       $("body").css("width",'99%');
    }
});

window.onbeforeunload = function(){
    window.clearInterval(video_status_times);
}