$(document).ready(function(){
  var lf = $.cookie("loginflag_"+g_hostname);
  if (null === lf || typeof(lf) =='undefined' || lf === "null" || 0 > parseInt(lf)){
    self.location.href = "/login.asp";
  }else{
      _init_language()
      _init_load()
      _init_click()
    if(g_is_msie){
      $("#objects").html('<object id="PlayBack" name="PlayBack" CLASSID="CLSID:94bf05ba-8286-421e-8474-dfb09ad161a1" width="100%" height="100%"></object>');
    }else{
      $("#objects").html('<object id="PlayBack" name="PlayBack" type="application/npipcpb" width="100%" height="100%"></object>');
    }
    window.document.title = IDC_PLAYBACK;
    showLogo();
  }
})

var local_path = ""; //本地设置路径
var updateTimeHandle = 0;
var searchType = 0; //搜索类型
var wndChnIndex = 0;  // 当前窗口通道号
initPlayingVideo = function(path){
    open_flag[wndChnIndex] = true;
    snap_flag[wndChnIndex] = true;
    pause_flag[wndChnIndex] = false;
    audio_flag[wndChnIndex] = false;
    play_time[wndChnIndex] = parseInt(document.PlayBack.GetFileLength(wndChnIndex));
    $("#playVideo").attr("src","/image/playback/Pause.png");
    $("#playVideo").attr("title",IDC_TITLE_PAUSE);
    $("#volume").attr("src","/image/playback/volume.png"); 
    $("#volume").attr("title",IDC_VOLUME_OPEN);
}

OnPlay = function(time)
{   
   var length = play_time[wndChnIndex];
   $( "#sliderplayback" ).slider( "value", time*100/length);
   $(".time_start").html(FormatTime(parseInt(time)))
   $(".time_end").html(FormatTime(length))
}

updateSlider = function()
{
    window.clearTimeout(updateTimeHandle);
    updateTimeHandle = setTimeout('updateSlider()', 1000);
    if(open_flag[wndChnIndex]==false || pause_flag[wndChnIndex]==true){
      return;
    }
    var time = parseInt(document.PlayBack.GetPlayPosition(wndChnIndex));
    OnPlay(time);
}

resize_win = function(){
  var sleft = ($(window).width()-980)/2;
  if(sleft < 0){
    $("#centerDiv").css("margin-left",0);
    $("#centerDiv").css("width",980);
  }else{
    $("#centerDiv").css("margin-left",sleft);
  }
  var top = ($(window).height()-540-40-65)/2; 
  top = top < 0 ?0: top;
  $("#centerDiv").css("margin-top",top);
}

$(window).resize(function(){
  resize_win();
});

_init_load = function(){
  resize_win();
    $( "#sliderplayback" ).slider({
          orientation: "horizontal",
          range: "min",
          animate: true,
        min:0,
        max:100,
        stop: function(event, ui) { 
          if(open_flag[wndChnIndex]==false){
              $("#sliderplayback").slider("value", 0);
          }else{
              OnChangeSlider(event, ui);
          }
        }
     });

  $("#videotime").val(new_date());
  var language= GetCookieByKey("languages")==0?'zh-cn':'en';
  if(language == 'zh-cn' && g_lan_js_arr[0] == 'russian.js'){
    language = 'rus';
  }
  var webdeflang = GetCookieByKey("webdeflang");
  if (parseInt(webdeflang) == 1) {
     language = 'en';
  }
  $("#videotime").click(function(){
      WdatePicker({readOnly:true,lang:language,dateFmt:'yyyy-MM-dd'});
  });
  updateTimeHandle = setTimeout('updateSlider()', 1000); 
}

OnChangeSlider = function(event, ui)
{
    var length = play_time[wndChnIndex];
    var pos = length * ui.value / 100

    $(".time_start").html(FormatTime(parseInt(pos)))
    $(".time_end").html(FormatTime(length))
    PlayBack.SetPlayPosition(parseInt(pos), wndChnIndex);
    updateSlider();

}

_init_click = function(){
    $("#setBtn").click(function(){
      location.href = "index.asp"
    })

      $("#videoBtn").click(function(){
          location.href = "mainview.asp"
      })

      $("#exitBtn").click(function(){
        if(confirm(IDC_MSGBOX_MSG)){
           Set_cookie("curr_menu_id",-1);
           deleteCookie("loginflag_"+g_hostname);
           location.href = "../login.asp"
        }else{
          window.focus();
        }
      })

      $(document).on('mouseenter', '#result_tab tbody tr',function(){
          $(this).css("background","#121212")
      })

      $(document).on('mouseleave', '#result_tab tbody tr',function(){
          $(this).css("background","#2c2c2c")
      })

      $(document).on('dblclick', '#result_tab tbody tr',function(){
        if (open_flag[wndChnIndex]==true)
        {
            var ret =  PlayBack.Stop(wndChnIndex);
            if(ret){
              $("#sliderplayback" ).slider( "value", -1);
              $(".time_start").html(FormatTime(parseInt(0)));
              $(".time_end").html(FormatTime(play_time[wndChnIndex]));
             
              play_time[wndChnIndex] = 0;
              open_flag[wndChnIndex] = false;
              pause_flag[wndChnIndex] = false;
              showVideo($(this).text());
            }
        }else{
            showVideo($(this).text());
        }
        
      });

    $("#volume").click(function(){
        if(wndChnIndex == -1){
          return false;
        }
        if(open_flag[wndChnIndex] == false){
          return false;
        }
        if(audio_flag[wndChnIndex] == false){
  
          //如果其他通道声音打开，要先关闭声音
          for(var i=0;i<4;i++){
            if(audio_flag[i] == true){
              PlayBack.SetAudioOut(false, i);
            }
          }
         
          PlayBack.SetAudioOut(true, wndChnIndex);
          audio_flag[wndChnIndex] = true;
          
          $("#volume").attr("src","/image/playback/volumeOpen.png");
          $("#volume").attr("title",IDC_VOLUME_CLOSE);
        }else{
          PlayBack.SetAudioOut(false, wndChnIndex);
          audio_flag[wndChnIndex]  = false;
          
          $("#volume").attr("src","/image/playback/volume.png");
          $("#volume").attr("title",IDC_VOLUME_OPEN);
        }
    });

    $("#snapVideo").click(function(){
       if(open_flag[wndChnIndex]==true){
          snapVideo();
       }
    });

    $("#playVideo").click(function(){
      if(open_flag[wndChnIndex]==true){
           playOrPauseVideo();
      }
    });

    $("#stopVideo").click(function(){
      if(open_flag[wndChnIndex]==true){
           stopVideo();
      }
    });
    $("#fastVideo").click(function(){
      if(open_flag[wndChnIndex]==true){
          fastVideo();
      }
    });
    $("#slowVideo").click(function(){
      if(open_flag[wndChnIndex]==true){
          slowVideo();
      }
    });

    $("#openVideo").click(function(){
        audio_flag[wndChnIndex] = false;
        $("#volume").attr("src","/image/playback/volume.png");
        $("#volume").attr("title",IDC_VOLUME_OPEN);
        openVideo();
    });
}

function showVideo(path){
    wndChnIndex = document.PlayBack.GetSelectPlayPane();
    if(path.length>0){
      if(0<=searchType && searchType<=2){
          path = local_path + path; 
          PlayBack.SetFastPlayRate(1, wndChnIndex);
          var ret = PlayBack.OpenFile(path,wndChnIndex); 
          if(ret == false){
            return;
          }
      }
      else{
          for(var i =0; i < dev_arr.length;i++){
              if(dev_arr[i].indexOf(path) > -1)
              {
                  PlayBack.SetFastPlayRate(1, wndChnIndex);
                  var ret = PlayBack.OpenVodFile($.cookie("url"), $.cookie("rtspport"), $.cookie("user"), $.cookie("passwd"), dev_arr[i], wndChnIndex);
                  if(ret == false){
                    return;
                  }
                  break;
              }
          }  
      }
      initPlayingVideo(path);
    }
}

function snapVideo(){
    if(snap_flag[wndChnIndex]==true){
        LocalPath = GetCookieByKey("RecPath");
        if (LocalPath == -1)
        {
            LocalPath = "D:\\IPCamera";
        }
        var path = LocalPath+"\\snaplayback\\" + new_date() + "\\" + document.domain + "-1-stream1-" + video_times()+".jpg"
        if(PlayBack.SnapPic(path,wndChnIndex))
        {
          alert(IDC_PIC_SAVEOK+ " "+path+" "+IDC_PIC_DIR);
        }
    }
}



function _time_judge(start_h,start_m,start_s,stop_h,stop_m,stop_s){
      if(isNaN(start_h) || start_h > 23)
       {
            $("#start_h").val('00')
            alert(IDC_TIME_PROMPT);
            return false;
       }
       else if( isNaN(start_m) || start_m > 59){
            alert(IDC_TIME_PROMPT);
            $("#start_m").val('00')
            return false;
       }
       else if(start_s > 59 || isNaN(start_s)){
            alert(IDC_TIME_PROMPT);
            $("#start_s").attr('value','00')
            return false;
       }
       else if(stop_h > 23 || isNaN(stop_h)){
            alert(IDC_TIME_PROMPT);
            $("#stop_h").attr('value','23')
            return false;
       }
       else if(stop_m > 59 || isNaN(stop_m)){
            alert(IDC_TIME_PROMPT);
            $("#stop_m").val('59')
            return false;
       }
       else if(stop_s > 59 || isNaN(stop_s)){
            alert(IDC_TIME_PROMPT);
            $("#stop_s").val('59')
            return false;
       }
       else
            return true;
}

function Search(){
  $("#searchTip").remove();
  window.clearTimeout(searchTimeHandle);
  window.clearTimeout(continueSearchTimeHandle);
  dev_arr = [];
    var date = $("#videotime").val();
    $("#result_tab tbody tr").remove()
    var start = " " +times($("#start_h").val()) + ":" + times($("#start_m").val()) + ":" + times($("#start_s").val());
    var stop = " " +  times($("#stop_h").val())  + ":" + times($("#stop_m").val())  + ":" + times($("#stop_s").val());
  if(start.indexOf("false") > -1)
  {
      alert(IDC_TIME_PROMPT);
      window.focus();
      return;
  }

  var a = _time_judge($("#start_h").val(),$("#start_m").val(),$("#start_s").val(),$("#stop_h").val(),$("#stop_m").val(),$("#stop_s").val())
  if(!a){ return}

  if(start > stop){
      alert(IDC_PLAYBACK_TIME);
      window.focus();
      return;
  }

  if(start.indexOf("-") > -1 || stop.indexOf("-") > -1)
  {
      alert(IDC_TIME_PROMPT);
      window.focus();
      return;
  }
  frontSearchNum = 0;
  $("#search").attr("disabled",true);
  $("#sresult").after("<span style='color:red;' id='searchTip'>"+IDC_QUERYING+"</span>");
    var v = $("#video_option").val();
  searchType  = document.all.video_option.selectedIndex;
  window.focus();
  if(0<=searchType && 2>=searchType){
     local_record(date,v);
  }else{
     front_record(date,start,stop,v);
  }

  window.focus();
}


function local_record(date,type){
    var LocalPath = GetCookieByKey("RecPath");
  if (LocalPath == -1)
  {
      LocalPath = "D:\\IPCamera";
  }
    local_path = LocalPath+"\\record\\" + document.domain + "\\"+ date.replace(/-/g,"") + "\\";
    var start = times($("#start_h").val()) + times($("#start_m").val()) + times($("#start_s").val());
    var stop = times($("#stop_h").val()) + times($("#stop_m").val()) + times($("#stop_s").val());
  PlayBack.SearchFile(local_path,searchType,start,stop);
  $("#searchTip").remove();
  $("#search").attr("disabled",false);
}

var frontSearchNum = 0; //前端搜索录像数
var searchTimeHandle;  //搜索按钮定时器（查询2秒没有回应则设置可再次查询）
var continueSearchTimeHandle; //前端查询定时器（每次查询40条记录，直到查询完为止，再次点击搜索按钮可清除定时器）
var searchArr = ["",""]; //查询条件数组
var dev_arr; //查询结果数组
var continueFlag = 0; //继续查询标志

function front_record(date,start,stop,type){
    searchTimeHandle = setTimeout('updateSearchStatus()', 2000);
    searchArr[0] = "searchfilecfg -act list -research ";
    searchArr[1] = " -type "+parseInt(type)+" -suffix 2 -starttime '" + date + start + "' -endtime  '" + date + stop + "' -itemnum 40";
    GetJCP({cmd: searchArr.join("1"),ParseJCP: function(result){
        if(result == "Error"){
            $("#searchTip").remove();
            $("#search").attr("disabled",false);
            return false;
        } else {
            front_append_tab(result, type);
        }
    }})
}

function updateSearchStatus(){
    $("#searchTip").remove();
    $("#search").attr("disabled",false);
}

/**
 * 判断视频文件在对应时间段内是否有告警
 * @param {string} filename - 视频文件名，格式如 "S-085725-0898.mp4"
 * @param {string} recflag - 1440位的标志位字符串，每个字符代表一分钟的告警状态
 * @returns {boolean} - 返回true表示有告警，false表示无告警
 */
function is_alarm_video(filename, recflag) {
    // 验证输入参数
    if (typeof recflag !== 'string' || recflag.length !== 1440) {
        throw new Error('标志位字符串必须为1440个字符');
    }

    if (!filename || typeof filename !== 'string') {
        throw new Error('文件名不能为空');
    }

    // 从文件名中提取时间信息
    const timeMatch = filename.match(/S-(\d{6})-(\d+)\.mp4/);
    if (!timeMatch) {
        throw new Error('文件名格式不正确，应为 S-085725-0898.mp4 格式');
    }

    const startTimeStr = timeMatch[1]; // 085725
    const duration = parseInt(timeMatch[2]); // 898

    // 解析起始时间
    const hours = parseInt(startTimeStr.substring(0, 2));
    const minutes = parseInt(startTimeStr.substring(2, 4));
    const seconds = parseInt(startTimeStr.substring(4, 6));

    // 计算起始时间在一天中的分钟数[8](@ref)
    const startTotalMinutes = hours * 60 + minutes;

    // 计算结束时间（起始时间 + 时长）
    const endTotalSeconds = hours * 3600 + minutes * 60 + seconds + duration;
    const endTotalMinutes = Math.floor(endTotalSeconds / 60);

    // 检查标志位字符串中对应时间段是否有告警
    for (let minute = startTotalMinutes; minute <= endTotalMinutes; minute++) {
        // 处理跨天情况
        const actualMinute = minute % 1440;
        if (recflag[actualMinute] === 'A') {
            return true; // 发现告警
        }
    }

    return false; // 未发现告警
}

function front_append_tab(result, type){
    window.clearTimeout(searchTimeHandle);
    var arr = result.resultlist.split("#");
    continueFlag = result.complete;
    var str = "";
    for(var i = 0; i < arr.length-1; i++)
    {
        if(arr[i] == ""){ continue; }
        if (type == 1 && is_alarm_video(arr[i], result.recflag)) {
            continue; // 搜索定时录像, 告警录像直接 continue, 不显示
        } else if (type == 4 && !is_alarm_video(arr[i], result.recflag)) {
            continue; // 搜索告警录像, 定时录像直接 continue, 不显示
        }

        var b = arr[i].split("/");
        str += "<tr><td align='left'><span style='margin-left:2px;'>"+ b [b.length-1] + "</span></td></tr>";
        if (result.mp4dir != undefined)
        {
            dev_arr.push(result.mp4dir + arr[i]); 
        } else {
            dev_arr.push(arr[i]);
        }
          
        frontSearchNum++;
    }
    
    $("#result_tab tbody").append(str);
    $("#search").attr("disabled",false);
    continueSearchTimeout();
}

function continueSearchTimeout(){
  if(continueFlag == 0){
      cleanSearchStatus();
  }else{
      GetJCP({cmd: searchArr.join("0"),ParseJCP: function(rst){
        if(rst == "Error"){
          continueFlag = 0;
          cleanSearchStatus();
          return false;
        }
        else{
            var a = rst.resultlist.split("#");
            continueFlag = rst.complete;
            var str = "";
            for(var i = 0; i < a.length; i++)
            {
                if(a[i] == ""){
                  continue; 
                }
                var b = a[i].split("/");
                str += "<tr><td align='left'><span style='margin-left:2px;'>"+ b [b.length-1] + "</span></td></tr>";

                if (rst.mp4dir != undefined)
                {
                    dev_arr.push(rst.mp4dir + a[i]); 
                } else {
                    dev_arr.push(a[i]);
                }

                frontSearchNum++;        
            }
            $("#result_tab tbody").append(str); 
            $("#searchTip").html("<span style='color:red;' id='frontSearchNum'>("+frontSearchNum+")"+IDC_QUERYING+"</span>"); 
            if(continueFlag == 1){
              continueSearchTimeHandle = setTimeout('continueSearchTimeout()', 1000);
            }else{
              cleanSearchStatus();
            }
            
        }
      }});
  }
  
}

function cleanSearchStatus(){
      $("#searchTip").html("<span style='color:red;' id='searchTip'>("+frontSearchNum+")</span>");
      $("#search").attr("disabled",false);
      window.clearTimeout(continueSearchTimeHandle);
}

function _init_language(){
    $("#video_type").html(IDC_VIDEO+IDC_TYPE)
    $("#search").html(IDC_SEARCH)
    $("#start_time").html(IDC_START+IDC_TIME)
    $("#end_time").html(IDC_END+IDC_TIME)
    $('#sresult').html(IDC_SEARCH+IDC_RESULT)
    $("#filetype").html(IDC_TYPE)
    $("#video_date").html(IDC_VIDEO+IDC_TIME_DATE)
    $("#search_video_lab").html(IDC_SEARCH+IDC_VIDEO)
    $("#exit").html(IDC_EXIT)
    $("#videoview").html(IDC_PLAYVIDEO)
  $("#set").html(IDC_PARAMETER_SET)

  $("#openVideo").attr("title",IDC_TITLE_OPEN);
  $("#playVideo").attr("title",IDC_TITLE_PLAY);
  $("#stopVideo").attr("title",IDC_TITLE_STOP);
  $("#slowVideo").attr("title",IDC_TITLE_SLOWER);
  $("#fastVideo").attr("title",IDC_TITLE_FASTER);
  $("#snapVideo").attr("title",IDC_SNAP);
  $("#volume").attr("title",IDC_VOLUME_OPEN);

}

var open_flag = [false, false, false, false];  //各通道打开标志
var snap_flag = [true, true, true, true];  //能抓怕图片标志，暂停时无法实现抓拍
var pause_flag = [false, false, false, false]; //各通道是否暂停标志
var audio_flag = [false, false, false, false]; //各通道声音是否打开
var play_time = [0, 0, 0, 0];      // 各通道播放文件的时间长度


//播放/暂停按钮事件
function playOrPauseVideo()
{
    if (pause_flag[wndChnIndex]==true)
    {
        $("#playVideo").attr("src","/image/playback/Pause.png");
        $("#playVideo").attr("title",IDC_TITLE_PAUSE);
        pause_flag[wndChnIndex] = false;
        snap_flag[wndChnIndex] = true;
        PlayBack.Pause(wndChnIndex);
    }
    else
    {
        $("#playVideo").attr("src","/image/playback/Play.png");
        $("#playVideo").attr("title",IDC_TITLE_PLAY);
        pause_flag[wndChnIndex] = true;
        snap_flag[wndChnIndex] = false;
        PlayBack.Pause(wndChnIndex);
    }   
}

//停止按钮事件
function stopVideo(){  
    if (open_flag[wndChnIndex]==false)
    {
        return;
    }
    PlayBack.Stop(wndChnIndex);

    $("#sliderplayback" ).slider( "value", -1);
    $(".time_start").html(FormatTime(parseInt(0)));
    $(".time_end").html(FormatTime(play_time[wndChnIndex]));
   
    play_time[wndChnIndex] = 0;
    open_flag[wndChnIndex] = false;
    pause_flag[wndChnIndex] = false;
    $("#playVideo").attr("src","/image/playback/Play.png");
    $("#playVideo").attr("title",IDC_TITLE_PLAY);

    if(audio_flag[wndChnIndex] = true){
       audio_flag[wndChnIndex] = false;
       $("#volume").attr("src","/image/playback/volume.png");
       $("#volume").attr("title",IDC_VOLUME_OPEN);
    }
     
}

//视频自动播放完成
function finishVideo(nWindow){
    if(wndChnIndex == nWindow){
        $("#sliderplayback" ).slider( "value", -1);
        $(".time_start").html(FormatTime(parseInt(0)));
        $(".time_end").html(FormatTime(play_time[nWindow]));
        $("#playVideo").attr("src","/image/playback/Play.png");
        $("#playVideo").attr("title",IDC_TITLE_PLAY);

        $("#volume").attr("src","/image/playback/volume.png");
        $("#volume").attr("title",IDC_VOLUME_OPEN);
    }
    play_time[nWindow] = 0;
    open_flag[nWindow] = false;
    pause_flag[nWindow] = false;
    audio_flag[nWindow] = false;
}

//文件播放快进加速
function fastVideo(){ 
    var speed = PlayBack.GetFastPlayRate(wndChnIndex);
    if (16 >= speed)
    {
        speed = speed * 2;
        var ret = PlayBack.SetFastPlayRate(speed, wndChnIndex);
        if (false == ret)
        {
            return false;
        }
    }
}


//文件播放快进减速
function slowVideo(){    
    var speed = PlayBack.GetFastPlayRate(wndChnIndex);
    if (0.0625 <= speed)
    {   
        speed = speed / 2;
        var ret = PlayBack.SetFastPlayRate(speed, wndChnIndex);
        if (false == ret)
        {
            return false;
        }
    }
}

//打开本地视频
function openVideo(){
    if (OpenFile() == true){
        openVideoCallBack();
    }
}

function openVideoCallBack(){
    open_flag[wndChnIndex] = true;       
    snap_flag[wndChnIndex] = true;
    
    $("#playVideo").attr("src","/image/playback/Pause.png");
    $("#playVideo").attr("title",IDC_TITLE_PAUSE);

    $("#sliderplayback").slider( "value", 0);
    $(".time_start").html(FormatTime(parseInt(0)));
    $(".time_end").html(FormatTime(play_time[wndChnIndex]));
}

//打开视频
function OpenFile()
{  
    var LocalPath = GetCookieByKey("RecPath");
    if (LocalPath == -1)
    {
        LocalPath = "D:\\IPCamera";
    }
    if(g_is_msie){
      var strFileName = PlayBack.GetOpenFileDialog(LocalPath);
      return OpenFileCallBack(strFileName);
    }else{
      //通过事件回掉
      PlayBack.GetOpenFileDialog(LocalPath);
    }
}

function OpenFileCallBack(strFileName){
     if(strFileName==''){
        return false;
      }
      var ret = PlayBack.OpenFile(strFileName, wndChnIndex);
      if (false == ret)// 控件打开文件失败
      {
          return false;
      }else{
          PlayBack.SetFastPlayRate(1, wndChnIndex);
      }
      
      play_time[wndChnIndex] = parseInt(document.PlayBack.GetFileLength(wndChnIndex));

      return true;
}

//通道切换 
function wndChgChn(nWindow)
{  
    if (nWindow == wndChnIndex)
    {
        return;
    }
    wndChnIndex = nWindow;

    //如果其他通道声音打开，要先关闭声音
    for(var i=0;i<4;i++){
        if(audio_flag[i] == true){
          PlayBack.SetAudioOut(false, i);
        }
    }
    if(audio_flag[wndChnIndex] == true){
        $("#volume").attr("src","/image/playback/volumeOpen.png");
        $("#volume").attr("title",IDC_VOLUME_CLOSE);
    }else{
        $("#volume").attr("src","/image/playback/volume.png");
        $("#volume").attr("title",IDC_VOLUME_OPEN);
    }


    // 播放状态更新
    if(open_flag[wndChnIndex]==false){
        $("#sliderplayback").slider( "value", 0);
        $(".time_start").html(FormatTime(parseInt(0)));
        $(".time_end").html(FormatTime(play_time[wndChnIndex]));
    }
    if (pause_flag[nWindow]==true)
    {
        var time = parseInt(document.PlayBack.GetPlayPosition(wndChnIndex));
        OnPlay(time);

        $("#playVideo").attr("src","/image/playback/Play.png");
        $("#playVideo").attr("title",IDC_TITLE_PLAY);
    }
    else
    {
        $("#playVideo").attr("src","/image/playback/Pause.png");
        $("#playVideo").attr("title",IDC_TITLE_PAUSE);

        
        if(audio_flag[wndChnIndex] == true){
          PlayBack.SetAudioOut(true, wndChnIndex);
        }
        
    }

}

function videoView(){
    Set_cookie("curr_menu_id",-1);
    location.href ='mainview.asp';
}

function paramSet(){
    Set_cookie("curr_menu_id",-1);
    location.href = "index.asp"
}

function logout(){
    location.href = "../login.asp"
}


window.onbeforeunload = function(){
   for(var i=0;i<4;i++){
      if(open_flag[i] == true){
         PlayBack.Stop(i);
      }
   }
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

//非IE下插件时间
function FireChannelEvent(msg,chn,str){
    if(msg == 4){ //切换窗口
      wndChgChn(chn);
    }else if(msg == 3){ //本地搜索
      if(null != str && str != ''){
        $("#result_tab tbody").append("<tr><td align='left'><span style='margin-left:2px;'>"+str+"</span></td></tr>")
      }
    }else if(msg == 1){ //播放结束
        finishVideo(chn);
    }else if(msg == 5){ //本地打开文件
      if(OpenFileCallBack(str)){
        openVideoCallBack();
      }

    }
}
