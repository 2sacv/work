var involume, outvolume;
var audioindef, audiooutdef;

$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        showAudioInfo();
    }
})
    
//音频设置
function showAudioInfo(){
    try{
  	    GetJCPList({cmd: "devaudioopt -act list", ParseJCP: ParseDevaudioOptCfg});
        GetJCP({cmd: "audiocfg -act list", ParseJCP: function(jcpobj){
            involume = new SliderModules({
                targetId: "involume",
                min: 1,
                max: 100
              }); 
              involume.create();
              involume.onchange = function () {
                  $('#tdInvolume').text(involume.getValue());
              };


            outvolume = new SliderModules({
                targetId: "outvolume",
                min: 1,
                max: 100
              }); 
              outvolume.create();
              outvolume.onchange = function () {
                  $('#tdOutvolume').text(outvolume.getValue());
              };
           
            involume.wsetValue(parseInt(jcpobj.involume));
            $("#tdInvolume").html(jcpobj.involume);

            outvolume.wsetValue(parseInt(jcpobj.outvolume));
            $("#tdOutvolume").html(jcpobj.outvolume);

            $("#codetype").val(jcpobj.codetype);
            $("#inputtype").val(jcpobj.inputtype);
            $("#amrbps").val(jcpobj.amrbps);
           
            $("input[name='rdAudioIN'][value=" + parseInt(jcpobj.inenable) + "]").attr("checked",true);
            SetDisableEnable(parseInt(jcpobj.inenable));
        }});
    }catch(E){ return E;}
}

function SetDisableEnable()
{
    var arg0 = arguments[0];
    if (typeof arg0 == "undefined")
    {
        return -1;
    }
    
    if (arg0 == 1)
    {
        $("#codetype").attr("disabled", false);
        $("#inputtype").attr("disabled", false);
        $("#divInvolume").hide();
        $("#divOutvolume").hide();
        $("#outvolume").show();
        $("#involume").show();
        $("#tdInvolume").show();
        $("#tdOutvolume").show();
        involume.wsetValue($("#tdInvolume").html());
        outvolume.wsetValue($("#tdOutvolume").html());
        //DisableEnableBPS();
    }
    else if (arg0 == 0)
    {
        $("#codetype").attr("disabled", true);
        $("#inputtype").attr("disabled", true);
        $("#outvolume").hide();
        $("#involume").hide();
        var ti = $("#tdInvolume");
        var to = $("#tdOutvolume");
        $("#divInvolume").text(ti.text()).show();
        $("#divOutvolume").text(to.text()).show();
        ti.hide();
        to.hide();
    }
}

function SaveAudioSet(){
    var jcpstr = "audiocfg -act set  -inenable " + ($('input:radio[name="rdAudioIN"]:checked').val());
    jcpstr += " -involume " + (involume.getValue()) + " -outvolume " + (outvolume.getValue());
    jcpstr += " -codetype " + parseInt($("#codetype").val());
     //+ " -amrbps " + $("#amrbps").val();
    jcpstr += " -inputtype " + parseInt($("#inputtype").val());
    GetJCP({cmd: jcpstr});
    
    parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
    window.focus();
    return;
}


function DefauleVolume(){
    involume.wsetValue(parseInt(audioindef[0]["def"]));
    outvolume.wsetValue(parseInt(audiooutdef[0]["def"]));
    $("#tdInvolume").html(audioindef[0]["def"]);
    $("#tdOutvolume").html(audiooutdef[0]["def"]);
    $("#divInvolume").text(audioindef[0]["def"]);
    $("#divOutvolume").text(audiooutdef[0]["def"]);
}

function ParseDevaudioOptCfg(jcpstr)
{
	try {
        var streamJson = JSON.parse(jcpstr.substring(0, jcpstr.length - 1));
        audioindef = streamJson["audioin"] || [];
        audiooutdef = streamJson["audioout"] || [];
    }
	catch (E){}
}
