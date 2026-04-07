
//与Ntive交互基础函数
//requst:       请求信息 一般包括 funcName 和不同请求的参数
//persistent:   OneWay
///onSuccess:   成功的回调函数
//onFialure:    失败的回调函数
function callNative (reqInfo,cbSucc,cbFail,bOnce) {
    reqJson = JSON.stringify(reqInfo);
	return window.nativeCall({
    request:reqJson,
    persistent:!bOnce,
    onSuccess:cbSucc,
    onFailure:cbFail
	});
}

function registObserver() {
    var reqInfo = {
        "funcName" : "browser_registObserver",
		"resizable":true,
		"hookopen":false,
		"minwidth":600,
		"minheight":600
    };
    callNative(reqInfo,onNativeMsg,onCallNativeFail,false)
}

function onNativeMsg(msg) {
    msgObj = JSON.parse(msg)
    if (msgObj.name == "onTabBrowserCreated") {
        onTabBrowserCreated(msgObj.arg)
    } else if (msgObj.name == "onTabBrowserClosed") {
        onTabBrowserClosed(msgObj.arg)
    } else if (msgObj.name == "onBroadCast") {
        onBroadCast(msgObj.senderid,msgObj.sendertag,msgObj.msg)
    } else if(msgObj.name == "onRecordStop") {
        onRecordStop(msgObj.arg)
    }
    else console.log(msg)
}

function onCallNativeSucc (res) {
    console.log(res)
}

function onCallNativeFail (code,msg) {
    console.log(code +" " + msg)
}

function onTabBrowserCreated(browserInfo) {
    console.log("onTabBrowserCreated " + JSON.stringify(browserInfo))
}

function onTabBrowserClosed(browserInfo) {
    console.log("onTabBrowserClosed " + JSON.stringify(browserInfo))
}

function onBroadCast(senderID,senderTag,msgJson) {
    console.log("onBroadCast from:[" + senderID + ":"+senderTag+"] msg:" + JSON.stringify(msgJson))
}

function minimizeWindow() {
    var reqInfo = {
        "funcName" : "wnd_min"
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function maxmizeWindow() {
    var reqInfo = {
        "funcName" : "wnd_max"
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function closeWindow() {
    var reqInfo = {
        "funcName" : "wnd_close"
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function getWindowPos() {
    var reqInfo = {
        "funcName" : "wnd_getpos"
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function setWindowPos() {
    var reqInfo = {
        "funcName" : "wnd_setpos",
        "pos" : {
            "left":200,
            "top":200,
            "width":800,
            "height":600
        }
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function createTabBrowser(tabBrowseruniqeTag,groupName) {
    var reqInfo = {
        "funcName"  : "browser_create",
        "url"       : "https://www.163.com",
        "tag"       : tabBrowseruniqeTag,
        "group"     : groupName,
        "sharecache": true,
        "active"    : true,
        "pos" : {
            "left":0,
            "top":336,
            "right":0,
            "bottom":0
        }
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function createIETabBrowser(tabBrowseruniqeTag,groupName) {
    var reqInfo = {
        "funcName"  : "browser_create",
        "url"       : "www.baidu.com",
        "tag"       : tabBrowseruniqeTag,
        "group"     : groupName,
        "sharecache": true,
        "active"    : true,
        "ie"		: true,
        "pos" : {
            "left":0,
            "top":336,
            "right":0,
            "bottom":0
        }
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function createTabBrowser2(tabBrowseruniqeTag,groupName) {
    var reqInfo = {
        "funcName"  : "browser_create",
        //"url"       : "www.qq.com",
        "url"       : "webtest/index.html",
        "tag"       : tabBrowseruniqeTag,
        "group"     : groupName,
        "asmargin"  : true,
        "active"    : true,
        "muteothers": true,
        "pos" : {
            "left":200,
            "top":36,
            "right":1,
            "bottom":300
        }
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

var startPosX = 200;
var startPosY = 200;

function onGetScreenSize(ret) {

  screenInfo = JSON.parse(ret)

  var toolWndWidth = 800;
  var toolWndHeight = 600;
  var fWndPosX = (screenInfo.width - toolWndWidth) / 2 + screenInfo.startx
  var fWndPosY = (screenInfo.height - toolWndHeight) / 2

  var reqInfo = {
        "funcName"  : "tool_create",
        "url"       : "www.baidu.com",
        "tag"       : "ToolWndTag",
        "standico"  : true,
        "sharecache":true,
        "top":true,
        "killfocusaction":"none",
        "windowless":true,
        "pos" : {
            "x":startPosX,
            "y":startPosY,
            "width":toolWndWidth,
            "height":toolWndHeight
        }
    };
    startPosX += 50;
    startPosY += 50;
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}
window.onbeforeunload = function() {
  return 'This is an onbeforeunload message.';
}

function createToolBrowser() {

	var reqInfo = {
        "funcName"  : "sys_getscreeninfo"
    }
    callNative(reqInfo,onGetScreenSize,onCallNativeFail,true)
}

function selectTabBrowser(tagName) {
    var reqInfo = {
        "funcName" : "browser_select",
        "tag":tagName,
        "muteothers":true
        };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}


function closeTabBrowser(uniqeTag) {
	alert("1111")
    var reqInfo = {
        "funcName" : "browser_close",
        "tag" : uniqeTag,
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function isTabBrowserShow(uniqeTag) {
    var reqInfo = {
        "funcName" : "browser_isshow",
        "tag" : uniqeTag,
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function isTabBrowserMute(uniqeTag) {
    var reqInfo = {
        "funcName" : "browser_ismute",
        "tag" : uniqeTag,
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function showTabBrowser(uniqeTag,bShow) {
    var reqInfo = {
        "funcName" : "browser_show",
        "show" : bShow,
        "tag" : uniqeTag,
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function muteTabBrowser(uniqeTag,bMute) {
    var reqInfo = {
        "funcName" : "browser_mute",
        "mute":bMute,
        "tag" : uniqeTag,
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function tabBrowserNavi(uniqeTag,targeURL) {
    var reqInfo = {
        "funcName" : "browser_navigate",
        "bysys":true,
        "tag" : uniqeTag,
        "url" : targeURL
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function reloadBrowser(uniqeTag,withCache) {
    var reqInfo = {
        "funcName" : "browser_reload",
        "tag" : uniqeTag,
        "cache" : withCache
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function clearBrowserCache(uniqeTag) {
    var reqInfo = {
        "funcName" : "browser_clearcache",
        "tag" : uniqeTag,
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function createNewBrowser(url,name,left,top,width,height){
    posInfo = "left="+left +",top=" +top + ",width=" + width + ",height=" + height
    window.open (url, name, posInfo)
}

var browserName = "name1"

function openNewBrowser(){
    createNewBrowser("index.html",browserName,-1,-1,800,600)
}

function closeNewWindow(){
    var reqInfo = {
        "funcName" : "browser_close",
        "tag" : "1stTag",
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function HideAllBrowser(){
    var reqInfo = {
        "funcName" : "browser_hidealltab",
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function broadCastMsg(msgJson) {
    var reqInfo = {
        "funcName" : "browser_broadcast",
        "msg" : msgJson,
		"target":1
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function testBroadCast() {
    var msgJson = {
        "msgName" : "t飞机哦123jiewo",
        "msgParam" : {
            "arg1":"测试",
            "arg2":2
        }
    };
    broadCastMsg(msgJson)
}

function showTopFull(uniqeTag) {
    var reqInfo = {
        "funcName" : "browser_fullscreen",
        "tag" : uniqeTag,
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function getAllTabStates() {
    var reqInfo = {
        "funcName" : "browser_gettabstates"
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function getBaseInfo() {
	 var reqInfo = {
        "funcName" : "sys_getinfo"
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function setBaseInfo(sid,cid,gid) {
	 var reqInfo = {
        "funcName" : "sys_setinfo",
        "sid":sid,
        "cid":cid,
        "gid":gid
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}



function testLocalStorage(){
   
   var reqInfo = {
        "funcName" : "sys_getlocalstorage",
        "key":""
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)

    var JValue = {
        "param1" : "1",
        "param2":2,
    };
   jValueStr = JSON.stringify(JValue);

    var reqInfo = {
        "funcName" : "sys_updatelocalstorage",
        "key":"test",
        "value":jValueStr
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)

    var reqInfo = {
        "funcName" : "sys_getlocalstorage",
        "key":"test"
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function testMemStorage(uniqeTag){
	getMemStorage(uniqeTag)
	 var tagExtraInfo = {
        "param1" : "test",
        "param2" : {
            "arg1":"test1",
            "arg2":2
        }
    };
	updateMemStorage(uniqeTag,tagExtraInfo)
	getMemStorage(uniqeTag)
}

function getMemStorage(uniqeTag) {
	var reqInfo = {
        "funcName" : "sys_getmemstorage",
		"tag":uniqeTag
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function updateMemStorage(uniqeTag,jValue) {
	var reqInfo = {
        "funcName" : "sys_updatememstorage",
		"tag":uniqeTag,
		"value":jValue
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function ShowToast(uniqeTag,jValue) {
	var reqInfo = {
        "funcName" : "sys_showtoast",
		"msg":"提示文ffewfew分为区分去字",
		"staytime":0,
		"fadetime":1,
		"margin" : {
            "left":0,
            "top":180,
            "right":0,
            "bottom":0
        }
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function SetSysShutDown(bAdd) {
	var reqInfo = {
        "funcName" : "sys_shutdown",
        "state":bAdd,
		"leftminites":2,
		"warningminites":1
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function QueryClock(bAdd) {
	 var reqInfo = {
        "funcName" : "sys_clock",
        "optype":"query",
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function SetClock(sOptType,sKey) {

    
    var curTime = new Date();
    var sMonth = curTime.getMonth() + 1;
    var curMinit = curTime.getMinutes() + 1;

    var sTime = curTime.getFullYear() +"-" + sMonth + "-" + curTime.getDate() + "T" + curTime.getHours()+":"+curMinit + ":00";

    var reqInfo = {
        "funcName" : "sys_clock",
        "optype":sOptType,
        "key":sKey,
        "title":"闹钟 传奇",
        "info":"9点攻城",
        "detail":"www.baidu.com",
        "btnName":"",
        "actionType":1,
        "daily":false,
        "time":sTime
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}


function UpdateVersion(url){
	var reqInfo = {
        "funcName" : "sys_verupdate",
        "url":url
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)

}

function TestBossKey() {
	var reqInfo1 = {
        "funcName" : "sys_gethotkey",
        "name": "bosskey"
    };
    callNative(reqInfo1,onCallNativeSucc,onCallNativeFail,true)

    var reqInfo2 = {
        "funcName" : "sys_sethotkey",
        "name":"bosskey",
        "key1": 16,  //shift
        "key2": 17,  //ctrl
        "key3": 119  //F8
    };

    callNative(reqInfo2,onCallNativeSucc,onCallNativeFail,true)
}

function PrintHotKey() {
    var reqInfo = {
        "funcName" : "sys_gethotkey",
        "name": "bosskey"
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)

    reqInfo.name = "keepclick";
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)

    reqInfo.name = "record";
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)

    reqInfo.name = "replay";
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)

    reqInfo.name = "topfull";
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function ExecuteProgram(exeName,param) {
    var reqInfo = {
        "funcName" : "sys_execute",
        "name": exeName,
        "param":param
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function RecordScreen(time,action)
{
    var reqInfo = {
        "funcName" : "sys_recordscreen",
        "time":time,
        "rate":40,
        "shrink":50,
        "action":action
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function SnapShot()
{
     var reqInfo = {
        "funcName" : "sys_snapshot",
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function ForceGC(){
	console.log(window.gc)
}

function TestAutoMemFree() {
    var reqInfo = {
        "funcName" : "sys_autofreemem",
        "checkInterval": 3,
        "freeInterval":60
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function GetMemUseSize() {
    var reqInfo = {
        "funcName" : "sys_getmemsize"
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function HardwareInfo() {
    var reqInfo = {
        "funcName" : "sys_hardwareinfo"
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}

function Login() {
	  var reqInfo = {
        "funcName" : "sys_login",
		"uid":"23fioejqop32p90gerwpoj"
    };
    callNative(reqInfo,onCallNativeSucc,onCallNativeFail,true)
}