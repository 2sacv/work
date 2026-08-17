$(function(){
    var lf = $.cookie("loginflag_"+g_hostname);
    if (null === lf)
    {
       parent.location.href = "/login.asp";
    }else{
        initUserManageInfo();
    }
})

var usersInfo;//所有用户信息
var currUserInfo;//当前用户信息
function initUserManageInfo(){
    usersInfo = new Array(); 
    currUserInfo = new Array; 
    if (init_call())
    {      
        parent.paramFailTip(IDC_MSGBOX_CONNECTFAIL);
    }

    $("input[name='authchk']").click(function(){
        var a = $("input[name='authchk']:checked").val()
        GetJCP({cmd: "authmode -act set -mode " + a,ParseJCP: function(result){
            if(result != "Error"){
                parent.paramSaveTip(IDC_MSGBOX_SAVEOK);
                parent.self.location.href = "/login.asp";        
            }
        }});
    })
}


function init_call(){             
    try
    {
        GetJCP({cmd: "authmode -act list", ParseJCP: function(jcpobj){
                $("input[name='authchk'][value="+parseInt(jcpobj.mode)+"]").attr("checked",true);       
        }});
        GetJCPList({cmd: "userpasswd -act list", ParseJCP: ParseUserListCfg});
    }
    catch(E){return E;}
}

function ParseUserListCfg(jcpstr)
{
    try
    {
        if (isBlank(jcpstr))
        {
            throw 1;
        }
        
        var userArr = jcpstr.split("#");
        var len = userArr.length;

        if(len>1){
            var usr, grp, ret;
            for (var i = 0; i < len-1; i++)
            {
                usr =  GetRtspKeyStr(userArr[i], 'user');
                ret = GetRtspKeyStr(userArr[i], 'group');
                if (ret == 0)
                {
                    grp = 'admin';        
                }
                else if (ret == 1)
                {
                    grp = 'operator';
                }
                else if (ret == 2)
                {
                    grp = 'user';
                }
                
                usersInfo.splice(UsersInfoIndex(usr, grp), 0, new Array(usr, grp));
            }
        }
        ResetTable();
    }catch(e){
    }
}

function UsersInfoIndex(user, group)
{
    // 用户信息数组排序
    var iUsers = 0;
    var iGroup = 0;

    for (var i = 0; i < usersInfo.length; i++)
    {
        if (usersInfo[i][1] == group)
        {// 同组查找
            if (usersInfo[i][0] < user)
            {
                iUsers = i;
            }
        }
        else if (usersInfo[i][1] < group)
        {
            iGroup = i;
        }
    }

    if (0 >= iUsers || usersInfo.length <= iUsers)
    {// 没有同组用户
        iUsers = iGroup;
    }

    return iUsers + 1;
}

function ResetTable()
{
    // 清空列表
    $("#userList tbody tr").each(function(){
            $(this).remove();
    });
    // 初始化列表
    var insert = "<tbody>";
    for (var i = 0; i < usersInfo.length; i++)
    {
        insert += '<tr align="center" style="height:25px;background:rgb(182,187,194)">';
        insert += '<td width="145" class="l-grid-cell">' + usersInfo[i][0] + '</td>';
        insert += '<td width="145" class="l-grid-cell">' + "********" + '</td>';
        insert += '<td width="145" class="l-grid-cell">' + usersInfo[i][1]+ '</td>';
        insert += '</tr>';
    }
    insert += "</tbody>";
    if(usersInfo.length>0){
        $("#userList").append(insert);
    }

     $("#userList tbody tr").click(function(){
            $(this).css("background-color", "rgb(49, 106, 197)").siblings().css("background-color", "rgb(182, 187, 194)");
            $(this).find("td").each(function(i){
                if (i == 0)
                {
                    //用户名
                    $("#uname").val($(this).text());
                }
                else if (i == 2)
                {
                    //用户组
                    $("#selGroup").val($(this).text());
                }
            });
        
    });

   
}

function CheckUser(szUser)
{
    // 用户名长度判断
    if (isBlank(szUser))
    {
        parent.paramFailTip(IDC_GEN_USER_NOEMPTY);
        return false;
    }

    if (31 < szUser.getBytes())
    {
        parent.paramFailTip(IDC_USERNAME_LENGTH);
        return false;
    }

    // 用户名只能包含字母或数字
    var patrn_shuzi = /^[0-9]$/;
    var patrn_zimu = /^[a-z]|[A-Z]$/;
    var s = szUser.split('');
    var i = 0;
    
    for (i = 0; i < s.length; i++)
    {
        if (!patrn_shuzi.exec(s[i]) && !patrn_zimu.exec(s[i]))
        {
            parent.paramFailTip(IDC_USERNAME_PATTEN);
            return false;
        }
    }

    return true;
}

function CheckPassword(szPassword)
{
    // 密码长度判断
    if (isBlank(szPassword))
    {
        parent.paramFailTip(IDC_GEN_PASSWORD_NOEMPTY);
        return false;
    }

    if (szPassword.length < 4 || 15 < szPassword.getBytes())
    {
        parent.paramFailTip(IDC_PASSWORD_LENGTH);
        return false;
    }
    
    // 密码必须是字母与数字的组合
    var patrn_shuzi = /^[0-9]$/;
    var patrn_zimu = /^[a-z]|[A-Z]$/;
    var flag_shuzi = 0;
    var flag_zimu = 0;
    var s = szPassword.split('');
    var i = 0;
    
    for (i = 0; i < s.length; i++)
    {
        if (!patrn_shuzi.exec(s[i]) && !patrn_zimu.exec(s[i]))
        {
            parent.paramFailTip(IDC_PASSWORD_PATTEN);
            return false;
        }
        
        if (patrn_shuzi.exec(s[i]))
        {
            flag_shuzi = 1;
        }
        
        if (patrn_zimu.exec(s[i]))
        {
            flag_zimu = 1;
        }
    }

    if (!flag_shuzi && !flag_zimu)
    {
        parent.paramFailTip(IDC_PASSWORD_PATTEN);
        return false;
    }
    
    return true;
}

// 增加用户
function AddUsr()
{
    if(usersInfo.length>=8){
        parent.paramFailTip(IDC_USRPASSWD_MAX);
        window.focus(); 
        return;
    }
    var user = $("#uname").val();
    var passwd = $("#passwd").val();
    var passwdok = $("#passwdok").val();
    
    // 用户名、密码合法性判断
    if (!CheckUser(user) || !CheckPassword(passwd))
    {
        window.focus(); 
        return false;
    }

    if (passwd!=passwdok){
        parent.paramFailTip(IDC_PASSWORD_NOTMATCH);
        window.focus(); 
        return false;
    }
    
    // 判断每个分组的用户数
    var grp;
    var ret = 0;
    var count = 0;
    for (var i = 0; i < usersInfo.length; i++)
    {        
        if (usersInfo[i][0] == user)
        {
            parent.paramFailTip(IDC_USERNAME_EXIST);
            window.focus(); 
            return false;
        }
        
        if (usersInfo[i][1] == $("#selGroup").val())
        {
            count++;
        }
    }
    
    if (count >= 8)
    {
        parent.paramFailTip(IDC_GROUP_USER_MAX);
        window.focus(); 
        return false;
    }

    // 增加用户
    switch($("#selGroup").val())
    {
        case "admin":
        {
            grp = 0;
            break;
        }
        case "operator":
        {
            grp = 1;
            break;
        }
        case "user":
        {
            grp = 2;
            break;
        }
    }
    
    try
    {               
        var jcpstr = "userpasswd -act add" + " -user " + user + " -password " + passwd + " -group " + grp;
        GetJCP({cmd: jcpstr});
        $("#uname").val("");
        $("#passwd").val("");
        $("#passwdok").val("");
        $("#selGroup").attr("value", "admin");
    }
    catch(e){}
    
    usersInfo.splice(UsersInfoIndex(user, $("#selGroup").val()), 0, new Array(user,  $("#selGroup").val()));
    ResetTable();
    
    parent.paramSaveTip(IDC_ADD_OK);

    window.focus();
    return true;
}

// 删除用户
var deluser = "";
function DelUsr()
{
    var user = $("#uname").val();
   
    var index = -1;
    var admincnt = 0;
    for (var i = 0; i < usersInfo.length; i ++)
    {
        // 判断是否只剩下一个admin用户
        if (usersInfo[i][1].toString() == "admin")
        {
            admincnt++;
        }
        // 判断用户名是否存在
        if (usersInfo[i][0].toString() == user){
            index = i;
        }
    }
    if (-1 == index)
    {
        parent.paramFailTip(IDC_USERNAME_NOT_EXIST);
        window.focus();
        return false;
    }

    if (admincnt <= 1 && usersInfo[index][1].toString() == "admin")
    {
        parent.paramFailTip(IDC_HOLD_ONE_ADMINUSER);
        window.focus();
        return false;
    }

    if(confirm(IDC_DEL_CONFIRM)){
        $("#userList").find("tbody>tr").each(function(i){
            if ($("#userList tbody>tr:eq(" + i + ") td:eq(0)").text() == user)
            {
                $("#userList tbody>tr:eq(" + i + ")").remove();
                $("#uname").val("");
                $("#passwd").val("");
                $("#passwdok").val("");
                $("#selGroup").val("admin");

                // 更新usersInfo中的信息
                usersInfo.splice(i, 1);
            }
        });
        
        // 发送JCP命令
        deluser = user;

        var jcpstr = "userpasswd -act del -user " + user;
        GetJCP({cmd: jcpstr, ParseJCP: function(a){
            parent.paramSaveTip(IDC_DEL_OK);
            if (deluser == GetCookieByKey("user"))
            {
                parent.self.location = "/login.asp";
            }
        }});
    }
    
    window.focus();
    return true;
}

// 修改用户名、密码、分组
function ModifyUsr()
{
    // 判断是否选中列表
    var index = -1;

    $("#userList").find("tbody>tr").each(function(i){
        var bc = $(this).css("background-color");
        if ("rgb(49, 106, 197)" == bc || (bc.indexOf("#")==-1 && "rgb(49, 106, 197)" == RGB2HEX(bc)))
        {
            index = i;
        }
    });

    if (-1 == index)
    {
        parent.paramFailTip(IDC_MODIFY_MSG_EMPTY);
        window.focus();
        return false;
    }

    var user = $("#uname").val();
    var passwd = $("#passwd").val();
    var passwdok = $("#passwdok").val();
    var group = $("#selGroup").val();

    if(usersInfo[index][0].toString() != user){
        parent.paramFailTip(IDC_USRPASSWD_NOT_MODIFY);
        window.focus();
        return false;
    }

    // 用户名、密码合法性判断
    if (!CheckUser(user) || !CheckPassword(passwd))
    {
        window.focus();
        return false;
    }
    if (passwd!=passwdok){
        parent.paramFailTip(IDC_PASSWORD_NOTMATCH);
        window.focus();
        return false;
    }


    //判断用户名是否已经存在
    var userSameFlag = false;
    for (var i = 0; i < usersInfo.length; i ++)
    {
        if (usersInfo[i][0].toString() == user && index != i)
        {
            userSameFlag = true;
            break;
        }
    }

    if(userSameFlag){
        parent.paramFailTip(IDC_USERNAME_EXIST);
        window.focus();
        return false;
    }

    // 判断是否改变
    /*for (var i = 0; i < usersInfo.length; i++)
    {        
        if (usersInfo[i][0] == user
            && usersInfo[i][1] == group)
        {
            parent.paramFailTip(IDC_MODIFY_MSG_SAME);
            return false;
        }
    }*/

    // 判断是否只剩下一个admin用户
    if (group !== usersInfo[index][1] && "admin" == usersInfo[index][1])
    {
        var admincnt = 0;
        for (var i = 0; i < usersInfo.length; i ++)
        {
            if (usersInfo[i][1].toString() == "admin")
            {
                admincnt++;
            }
        }

        if (admincnt <= 1)
        {
            parent.paramFailTip(IDC_HOLD_ONE_ADMINUSER);
            window.focus();
            return false;
        }
    }

    // 转换用户组
    var grp = 0;
    switch(group)
    {
        case "admin":
        {
            grp = 0;
            break;
        }
        case "operator":
        {
            grp = 1;
            break;
        }
        case "user":
        {
            grp = 2;
            break;
        }
    }
                           //userpasswd -act set -user admin1 -password admin1
    // 保存参数
    currUserInfo["jcpstr"] = "userpasswd -act set" + " -user  " + user + " -password " + passwd + " -group " + grp;;
    currUserInfo["index"] = index;
    currUserInfo["user"] = user;
    currUserInfo["passwd"] = passwd;
    currUserInfo["group"] = group;
    currUserInfo["grp"] = grp;

   
    ModifyUsrInfo();
}


function ModifyUsrInfo()
{
    var jcpstr = currUserInfo["jcpstr"];
    var index = currUserInfo["index"];
    var user = currUserInfo["user"];
    var passwd = currUserInfo["passwd"];
    var grp = currUserInfo["grp"];
    var group = currUserInfo["group"];

    // 发送JCP命令
    //GetJCP({cmd: jcpstr,ParseJCP:});
    GetJCPList({cmd:jcpstr , logType:1,ParseJCP: function(jcpstr){
        var result = GetRtspKeyStr(jcpstr,"result");
        if(-3 == result){
            parent.paramFailTip(IDC_USERNAME_NOT_EXIST);
            $("#userName").focus();
            return false;
        }else{
            if (GetCookieByKey("user") == usersInfo[index][0])
            {
                Set_cookie("group", grp);
                parent.self.location = "/login.asp";
            }

            usersInfo.splice(index, 1);
            usersInfo.splice(UsersInfoIndex(user, group), 0, new Array(user, group));
            ResetTable();
            $("#uname").val("");
            $("#passwd").val("");
            $("#passwdok").val("");
            $("#selGroup").attr("value", "admin");
            parent.paramSaveTip(IDC_MODIFY_USRINFO_OK);
            window.focus();
        }
    }});
   
}