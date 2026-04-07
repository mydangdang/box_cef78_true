#include "WndOpt.h"
#include "tests/cefclient/browser/test_runner.h"
#include "tests/cefclient/browser/main_context_impl.h"
#include "../browser/root_window_win.h"
#include "../string_util.h"
#include "../Alarms.h"
#include "../RTCTX.h"
#include "tests/shared/browser/util_win.h"
#include <shlwapi.h>
#include <ShlObj.h>
#include "../UtilLocalStorage.h"
#include "../HotKeyManager.h"
#include "../MemoryDeal.h"
#include "../WinHttpClient.h"
#include "../ScreenRecorder.h"

namespace client {
namespace WndOpt {

	namespace {


		const char kNameKey[] = "funcName";

		// Handle messages in the browser process.
		class Handler : public CefMessageRouterBrowserSide::Handler {
		public:
			Handler() {
				//把 appdata中的 updater删掉;
				std::string szPath = client::DataPath();
				std::string szUpdater = szPath + "\\updater.exe";
				if (PathFileExistsA(szUpdater.c_str()))
					DeleteFileA(szUpdater.c_str());
			}

			virtual bool OnQuery(CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame,
				int64 query_id,
				const CefString& request,
				bool persistent,
				CefRefPtr<Callback> callback) OVERRIDE {

		
				CefRefPtr<CefDictionaryValue> request_dict = ParseJSON(request);
				if (!request_dict) {
					callback->Failure(1, "Incorrect Request format");
					return true;
				}

				if (!VerifyKey(request_dict, kNameKey, VTYPE_STRING, callback))
					return true;

				const std::string& funcName = request_dict->GetString(kNameKey);
				bool bDeal = false;

				CefRefPtr<CefDictionaryValue> resp = CefDictionaryValue::Create();
				if (funcName == "wnd_min") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					HWND hBrowserWnd = browser->GetHost()->GetWindowHandle();
					HWND hHostWnd = GetParent(hBrowserWnd);
					if (hHostWnd) PostMessage(hHostWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
					else PostMessage(hBrowserWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
					
					resp->SetString("msg", "succ");
				}
				else if (funcName == "wnd_max") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					HWND hBrowserWnd = browser->GetHost()->GetWindowHandle();
					HWND hHostWnd = GetParent(hBrowserWnd);
					if (hHostWnd)
					{
						if (IsZoomed(hHostWnd)) PostMessage(hHostWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
						else PostMessage(hHostWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
					}
					else
					{
						if (IsZoomed(hBrowserWnd)) PostMessage(hBrowserWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
						else PostMessage(hBrowserWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
					}
					resp->SetString("msg", "succ");
				} 
				else if (funcName == "wnd_close") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					HWND hBrowserWnd = browser->GetHost()->GetWindowHandle();
					HWND hHostWnd = GetParent(hBrowserWnd);
					if(hHostWnd) PostMessage(hHostWnd, WM_CLOSE, browser->GetIdentifier(), 0);
					else PostMessage(hBrowserWnd, WM_CLOSE, browser->GetIdentifier(), 0);
					
					resp->SetString("msg", "succ");
				} 
				else if (funcName == "wnd_getpos") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					HWND hHostWnd = GetParent(browser->GetHost()->GetWindowHandle());
					RECT rcWindow = { 0 };
					GetWindowRect(hHostWnd,&rcWindow);

					CefRefPtr<CefDictionaryValue> wndPos = CefDictionaryValue::Create();

					wndPos->SetInt("left", rcWindow.left);
					wndPos->SetInt("top", rcWindow.top);
					wndPos->SetInt("width", rcWindow.right-rcWindow.left);
					wndPos->SetInt("height", rcWindow.bottom-rcWindow.top);

					resp->SetDictionary("pos", wndPos);
					resp->SetString("msg", "succ");
				} 
				else if (funcName == "wnd_setpos") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					CefRefPtr<CefDictionaryValue> wndPos = request_dict->GetDictionary("pos");
					if (!wndPos) {
						resp->SetString("msg", "not enought pos param");
					} else {
						RECT rcWindow = { 0 };
						rcWindow.left = wndPos->GetInt("left");
						rcWindow.top = wndPos->GetInt("top");
						rcWindow.right = wndPos->GetInt("width");
						rcWindow.bottom = wndPos->GetInt("height");
						
						HDC screen_dc = ::GetDC(NULL);
						int dpi_x = GetDeviceCaps(screen_dc, LOGPIXELSX);
						float fScal = static_cast<float>(dpi_x) / 96.0f;
						::ReleaseDC(NULL, screen_dc);

						rcWindow.right *= fScal;
						rcWindow.bottom *= fScal;

						if (rcWindow.left == -1) {
							rcWindow.left = (int)((GetSystemMetrics(SM_CXSCREEN) - rcWindow.right) * 1.0f / 2);
							rcWindow.top = (int)((GetSystemMetrics(SM_CYSCREEN) - rcWindow.bottom) * 1.0f / 2);
						}
				
						HWND hHostWnd = GetParent(browser->GetHost()->GetWindowHandle());
						SetWindowPos(hHostWnd, NULL, rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom, SWP_NOZORDER);

						resp->SetString("msg", "succ");
					}
				}
				else if (funcName == "sys_setinfo") {

					bDeal = true;
					resp->SetString(kNameKey, funcName);
					
					std::string szSid = request_dict->GetString("sid");
					std::string szCid = request_dict->GetString("cid");
					std::string szGid = request_dict->GetString("gid");
					
					MainContextImpl::Get()->SetInfo(szSid, szCid, szGid);
					
					resp->SetString("msg", "succ");
				}
				else if (funcName == "sys_getinfo") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					resp->SetString("msg", "succ");
					resp->SetString("ver", MainContextImpl::Get()->GetVersion());

					std::string szBaseJson = MainContextImpl::Get()->GetBaseInfo();
				
					CefRefPtr<CefValue> jBase = CefParseJSON(szBaseJson, JSON_PARSER_RFC);

					resp->SetValue("info", jBase);
				}
				else if (funcName == "sys_getscreeninfo") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					resp->SetString("msg", "succ");

					HWND hHostWnd = GetParent(browser->GetHost()->GetWindowHandle());
					MONITORINFO oMonitor = {};
					oMonitor.cbSize = sizeof(oMonitor);
					::GetMonitorInfo(::MonitorFromWindow(hHostWnd, MONITOR_DEFAULTTOPRIMARY), &oMonitor);

					resp->SetInt("startx", oMonitor.rcWork.left);
					resp->SetInt("width", GetSystemMetrics(SM_CXSCREEN));
					resp->SetInt("height", GetSystemMetrics(SM_CYSCREEN));
					resp->SetDouble("scale", GetWindowScaleFactor(hHostWnd));
				}
				else if (funcName == "sys_getmemstorage") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					std::string szTag = request_dict->GetString("tag");
					std::string szValue = MainContextImpl::Get()->GetMemStorage(szTag);
					if (szTag.empty() || szValue.empty())
					{
						resp->SetString("msg", "failed");
						callback->Success(GetJSON(resp));
						return true;
					}
					resp->SetString("msg", "succ");
					resp->SetString("tag", szTag);
					resp->SetString("value", szValue);
				}
				else if (funcName == "sys_updatememstorage") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					std::string szTag = request_dict->GetString("tag");
					if (szTag.empty())
					{
						resp->SetString("msg", "failed");
						callback->Success(GetJSON(resp));
						return true;
					}
					resp->SetString("msg", "succ");
					std::string szValue = request_dict->GetString("value");
					MainContextImpl::Get()->UpdateMemStorage(szTag, szValue);
				}
				else if (funcName == "sys_setlocalstorage") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					bool bSucc = LSHelper.SetLocalStorage(request_dict->GetString("value").ToWString());
					if (!bSucc)
					{
						resp->SetString("msg", "failed");
						callback->Success(GetJSON(resp));
						return true;
					}
					resp->SetString("msg", "succ");
				}
				else if (funcName == "sys_getlocalstorage") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					std::wstring szValue = LSHelper.GetLocalStorage(request_dict->GetString("key").ToWString());
					resp->SetString("msg", "succ");
					resp->SetString("key", request_dict->GetString("key"));
					resp->SetString("value", szValue);
				}
				else if (funcName == "sys_updatelocalstorage") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					std::wstring szKey = request_dict->GetString("key").ToWString();
					std::wstring szValue = request_dict->GetString("value").ToWString();
					if (szKey.empty() || szValue.empty())
					{
						resp->SetString("msg", "failed");
						callback->Success(GetJSON(resp));
						return true;
					}
					bool bSucc = LSHelper.UpdateLocalStorage(szKey, szValue);
					if (!bSucc)
					{
						resp->SetString("msg", "failed");
						callback->Success(GetJSON(resp));
						return true;
					}
					resp->SetString("msg", "succ");
				}
				else if (funcName == "sys_showtoast") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					std::wstring szMsg = request_dict->GetString("msg");
					int nStayTime = request_dict->GetInt("staytime");
					int nFadeTime = request_dict->GetInt("fadetime");

					CefRefPtr<CefDictionaryValue> wndPos = request_dict->GetDictionary("margin");
					if (!wndPos) {
						resp->SetString("msg", "pos param missing");
						resp->SetString("msg", "failed");
					}
					else {
						RECT rcWindow = { 0 };
						rcWindow.left = wndPos->GetInt("left");
						rcWindow.top = wndPos->GetInt("top");
						rcWindow.right = wndPos->GetInt("right");
						rcWindow.bottom = wndPos->GetInt("bottom");
						int nBrowserID = browser->GetIdentifier();


						CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(nBrowserID);
						if (rootWindow != nullptr) {
							RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
							pRootWindowWin->ShowToast(nBrowserID, szMsg, rcWindow, nStayTime, nFadeTime);
						}
						resp->SetString("msg", "succ");
					}

				}
				else if (funcName == "sys_shutdown")
				{
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					int nLeftMinites = request_dict->GetInt("leftminites");
					int nWarningMinites = request_dict->GetInt("warningminites");
					bool bState = request_dict->GetBool("state");
					bool bSucc = MainContextImpl::Get()->SysShutDownPlan(browser->GetIdentifier(),bState, nLeftMinites, nWarningMinites);
					if(bSucc) resp->SetString("msg", "succ");
					else resp->SetString("msg", "failed");

				}
				else if (funcName == "sys_clock")
				{
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					std::string szType = request_dict->GetString("optype");
					if (szType == "cancel")
					{
						std::wstring szTaskName = request_dict->GetString("key");
						Alarms::Instance()->DelAlarm(UtilString::SW2A(szTaskName));
						resp->SetString("msg", "succ");
					}
					else if (szType == "add")
					{
						bool bDaily = request_dict->GetBool("daily");
						std::string szTag = request_dict->GetString("observer");
						if (szTag.empty())
						{
							CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
							szTag = rootWindow->GetMainBrowserTag();
						}
						std::wstring szName = request_dict->GetString("key");
						std::wstring szTime = request_dict->GetString("time");
						std::wstring szInfo = request_dict->GetString("info");
						std::wstring szText = request_dict->GetString("title");
						std::wstring szDetail = request_dict->GetString("detail");
						std::wstring szAction = request_dict->GetString("btnName");
						int nType = request_dict->GetInt("actionType");
						
						bool bSuccess = Alarms::Instance()->AddAlarm(szTag, UtilString::SW2A(szName), UtilString::SW2A(szTime), UtilString::SW2A(szText),
							UtilString::SW2A(szInfo), UtilString::SW2A(szAction), UtilString::SW2A(szDetail), nType, bDaily);
						if(bSuccess) resp->SetString("msg", "succ");
						else  resp->SetString("msg", "failed");
					}
					else if (szType == "query")
					{
						std::vector<Alarms::AlarmItem> vAlarms = Alarms::Instance()->GetAlarms();

						CefRefPtr<CefListValue> alramlist = CefListValue::Create();
						for (UINT i = 0; i < vAlarms.size(); i++)
						{
							CefRefPtr<CefDictionaryValue> value = CefDictionaryValue::Create();
							std::wstring szName = UtilString::SA2W(vAlarms[i].szName);
							std::wstring szTime = UtilString::SA2W(vAlarms[i].szTime);
							std::wstring szInfo = UtilString::SA2W(vAlarms[i].szInfo);

							std::wstring szText = UtilString::SA2W(vAlarms[i].szText);
							std::wstring szDetail = UtilString::SA2W(vAlarms[i].szDetail);
							std::wstring szAction = UtilString::SA2W(vAlarms[i].szAct);
							std::wstring szObserver = UtilString::SA2W(vAlarms[i].szObserver);

							value->SetString("key", szName.c_str());
							value->SetString("time", szTime.c_str());
							value->SetString("info", szInfo.c_str());

							value->SetString("title", szText.c_str());
							value->SetString("detail", szDetail.c_str());
							value->SetString("btnName", szAction.c_str());
							value->SetString("observer", szObserver.c_str());
							value->SetInt("actionType", vAlarms[i].nActType);
							value->SetBool("daily", vAlarms[i].bDaily);
							alramlist->SetDictionary(i, value);
						}
						resp->SetList("alarms", alramlist);
						resp->SetString("msg", "succ");
					}
					else
					{
						resp->SetString("msg", "failed");
					}
				}
				else if (funcName == "sys_patch") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					std::wstring szName = request_dict->GetString("name");
					std::wstring szURL = request_dict->GetString("url");
					std::wstring szCmd = request_dict->GetString("cmd");

					std::string szPath = client::DataPath();
					std::wstring szPatchFile = UtilString::SA2W(szPath) + L"\\" + szName;
	
					bool bSucc = false;
					do 
					{
						WinHttpClient clt(szURL.c_str());
						bSucc = clt.SendHttpRequest();
						if(!bSucc) break;
						bSucc = clt.SaveResponseToFile(szPatchFile);
						if (!bSucc) break;
					} while (0);

					if (bSucc && PathFileExists(szPatchFile.c_str()))
					{
						SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};
						sei.fMask = SEE_MASK_NOCLOSEPROCESS;
						sei.lpVerb = L"runas";
						sei.lpFile = szPatchFile.c_str();
						sei.lpParameters = szCmd.c_str();
						sei.nShow = SW_SHOWNORMAL;
						bSucc = ShellExecuteEx(&sei);
						if (bSucc) resp->SetString("msg", "succ");
						else resp->SetString("msg", "failed");
					}
					else resp->SetString("msg", "failed");
	
				}
				else if (funcName == "sys_verupdate") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					CefRefPtr<CefDictionaryValue> processCMD = CefDictionaryValue::Create();
					processCMD->SetString("url", request_dict->GetString("url"));

					std::wstring szCurPath = UtilString::SA2W(RTCTX::CurPath());
					processCMD->SetString("path", szCurPath);
				
					CefRefPtr<CefValue> value = CefValue::Create();
					value->SetDictionary(processCMD);
					CefString scefCmd = CefWriteJSON(value, JSON_WRITER_DEFAULT);

					std::wstring wszCmd = scefCmd.ToWString();
					
					std::string szCmd = UtilString::SW2A(wszCmd);
					szCmd = UtilString::Base64Encdoe(szCmd);

					std::string szDataPath = client::DataPath();
					if (!PathFileExistsA(szDataPath.c_str())) SHCreateDirectoryExA(NULL, szDataPath.c_str(), 0);
					std::string szPath = RTCTX::CurPath();
					std::string szUpdaterFile = szPath + "\\updater.exe";
					std::string szUpdaterDst = szDataPath + "\\updater.exe";

					if (PathFileExistsA(szUpdaterDst.c_str())) DeleteFileA(szUpdaterDst.c_str());

					CopyFileA(szUpdaterFile.c_str(), szUpdaterDst.c_str(), FALSE);

					if (PathFileExistsA(szUpdaterDst.c_str()))
					{
						SHELLEXECUTEINFOA sei = { sizeof(SHELLEXECUTEINFO) };
						sei.fMask = SEE_MASK_NOCLOSEPROCESS;
						sei.lpVerb = "runas";
						sei.lpFile = szUpdaterDst.c_str();
						sei.lpParameters = szCmd.c_str();
						sei.nShow = SW_SHOWNORMAL;
						BOOL bSucc = ShellExecuteExA(&sei);
						if (bSucc) resp->SetString("msg", "succ");
						else resp->SetString("msg", "failed");
					}
					else resp->SetString("msg", "failed");
				}
				else if (funcName == "sys_login")
				{
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					std::string szUID = request_dict->GetString("uid");
					int nMinits = request_dict->GetInt("interval");
					MainContextImpl::Get()->GetBaseInfo();
					MainContextImpl* pMain = (MainContextImpl*)MainContextImpl::Get();
					resp->SetString("msg", "succ");
// 					std::string szDeamon = RTCTX::CurPath() + "\\BoxDeamon.exe";
// 					if(PathFileExistsA(szDeamon.c_str()))
// 					{
// 						std::string szCmd;
// 						std::string szSinglePipeName = pMain->m_szShareData + "Deamon";
// 						szCmd = "-p=";
// 						szCmd += std::to_string(GetCurrentProcessId());
// 						szCmd = szCmd + " -u=" + szUID;
// 						szCmd = szCmd + " -s=" + pMain->m_szSID;
// 						szCmd = szCmd + " -c=" + pMain->m_szCPUID;
// 						szCmd = szCmd + " -m=" + pMain->m_szMAC;
// 						szCmd = szCmd + " -h=" + pMain->m_szHost;
// 						szCmd = szCmd + " -n=" + szSinglePipeName;
// 						if (pMain->m_bEnableADS) szCmd = szCmd + " -t=" + std::to_string(nMinits);
// 
// 						SHELLEXECUTEINFOA sei = { sizeof(SHELLEXECUTEINFO) };
// 						sei.fMask = SEE_MASK_NOCLOSEPROCESS;
// 						sei.lpVerb = "open";
// 						sei.lpFile = szDeamon.c_str();
// 						sei.lpParameters = szCmd.c_str();
// 						sei.nShow = SW_SHOWNORMAL;
// 						BOOL bSucc = ShellExecuteExA(&sei);
// 						if (bSucc) resp->SetString("msg", "succ");
// 						else resp->SetString("msg", "failed");
// 					}
// 					else resp->SetString("msg", "failed");
				}
				/*兼容线上 bosskey代码*/
				else if (funcName == "sys_getbosskey") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					UINT uKey1 = 0;
					UINT uKey2 = 0;
					UINT uKey3 = VK_F9;
					stHotKey* pHotKey = hotKeyManager.GetHotKey(HK_BOSS);
					if (pHotKey)
					{
						uKey1 = pHotKey->uModifier1;
						uKey2 = pHotKey->uModifier2;
						uKey3 = pHotKey->uKey;
					}
					resp->SetInt("key1", uKey1);
					resp->SetInt("key2", uKey2);
					resp->SetInt("key3", uKey3);
					resp->SetString("msg", "succ");
				}
				else if (funcName == "sys_setbosskey") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					UINT uKey1 = request_dict->GetInt("key1");
					UINT uKey2 = request_dict->GetInt("key2");
					UINT uKey3 = request_dict->GetInt("key3");
		
					hotKeyManager.AddHotKey(HK_BOSS, uKey3, uKey2, uKey1, true);
					resp->SetString("msg", "succ");
				}
				/*兼容线上 bosskey代码*/
				else if (funcName == "sys_gethotkey") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					UINT uKey1 = 0;
					UINT uKey2 = 0;
					UINT uKey3 = 0;

					stHotKey* pHotKey = hotKeyManager.GetHotKey(request_dict->GetString("name"));
					if (pHotKey)
					{
						uKey1 = pHotKey->uModifier1;
						uKey2 = pHotKey->uModifier2;
						uKey3 = pHotKey->uKey;
					}
					resp->SetString("name", request_dict->GetString("name"));
					resp->SetInt("key1", uKey1);
					resp->SetInt("key2", uKey2);
					resp->SetInt("key3", uKey3);
					resp->SetString("msg", "succ");
				}
				else if (funcName == "sys_sethotkey") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					UINT uKey1 = request_dict->GetInt("key1");
					UINT uKey2 = request_dict->GetInt("key2");
					UINT uKey3 = request_dict->GetInt("key3");
					hotKeyManager.AddHotKey(request_dict->GetString("name"), uKey3, uKey2, uKey1, true);
					resp->SetString("msg", "succ");
				}
				else if (funcName == "sys_execute")
				{
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					resp->SetString("msg", "succ");
					std::wstring szExeName = request_dict->GetString("name");
					std::wstring wszCmd = request_dict->GetString("param");
					std::string szExePath = RTCTX::CurPath() + "\\" + UtilString::SW2A(szExeName);
					std::string szCmd = UtilString::SW2A(wszCmd);
					if (PathFileExistsA(szExePath.c_str()))
					{
						SHELLEXECUTEINFOA sei = { sizeof(SHELLEXECUTEINFO) };
						sei.fMask = SEE_MASK_NOCLOSEPROCESS;
						sei.lpVerb = "open";
						sei.lpFile = szExePath.c_str();
						sei.lpParameters = szCmd.c_str();
						sei.nShow = SW_SHOWNORMAL;
						BOOL bSucc = ShellExecuteExA(&sei);
						if (bSucc) resp->SetString("msg", "succ");
						else resp->SetString("msg", "failed");
					}
					else resp->SetString("msg", "failed");
				}
				else if (funcName == "sys_snapshot")
				{
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					resp->SetString("msg", "succ");
					std::string szExePath = RTCTX::CurPath() + "\\snapshot.exe";
					if (PathFileExistsA(szExePath.c_str()))
					{
						SHELLEXECUTEINFOA sei = {sizeof(SHELLEXECUTEINFO)};
						sei.fMask = SEE_MASK_NOCLOSEPROCESS;
						sei.lpVerb = "open";
						sei.lpFile = szExePath.c_str();
						sei.nShow = SW_SHOWNORMAL;
						BOOL bSucc = ShellExecuteExA(&sei);
						if (bSucc) resp->SetString("msg", "succ");
						else resp->SetString("msg", "failed");
					}
					else resp->SetString("msg", "failed");
				}
				else if (funcName == "sys_recordscreen")
				{
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					resp->SetString("msg", "succ");
					
					std::string szAction = "start";
					int nSecs = 300;
					int nRate = 40;
					int nShrink = 0;
					if (request_dict->HasKey("action"))
						szAction = request_dict->GetString("action");
					resp->SetString("action", szAction.c_str());

					bool bRet = true;
					if(szAction == "start")
					{
						if (request_dict->HasKey("time"))
							nSecs = request_dict->GetInt("time");
						if (request_dict->HasKey("rate"))
							nRate = request_dict->GetInt("rate");
						if (request_dict->HasKey("shrink"))
							nShrink = request_dict->GetInt("shrink");
						
						HWND hBrowserWnd = browser->GetHost()->GetWindowHandle();
						MONITORINFO oMonitor = {};
						oMonitor.cbSize = sizeof(oMonitor);
						::GetMonitorInfo(::MonitorFromWindow(hBrowserWnd, MONITOR_DEFAULTTONEAREST), &oMonitor);
						RECT rcWork = oMonitor.rcWork;

						CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
						bRet = ScreenRecorder::Instance()->Start(rootWindow->GetBrowserTagFromID(browser->GetIdentifier()), nSecs, nRate, nShrink, rcWork);
					}
					else bRet = ScreenRecorder::Instance()->Stop();

					if (bRet) resp->SetString("msg", "succ");
					else resp->SetString("msg", "failed");
				}
				else if (funcName == "sys_manulfreemem")
				{
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					resp->SetString("msg", "succ");
					MemoryDealer.FreeMeme();
				}
				else if (funcName == "sys_autofreemem")
				{
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					resp->SetString("msg", "succ");
					int nCheckInterval = request_dict->GetInt("checkInterval");
					int nFreeInterval = request_dict->GetInt("freeInterval");
					MemoryDealer.Start(nCheckInterval, nFreeInterval);
				}
				else if (funcName == "sys_getmemsize")
				{
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					resp->SetString("msg", "succ");
					resp->SetDouble("size", MemoryDealer.GetMemSize());
				}
				else if (funcName == "sys_hardwareinfo")
				{
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					std::wstring szInfo = MainContextImpl::Get()->GetHardwarInfo();
					CefRefPtr<CefValue> jBase = CefParseJSON(szInfo, JSON_PARSER_RFC);
					resp->SetValue("hardware", jBase);
				}
				if (bDeal) {
					callback->Success(GetJSON(resp));
				}
				return bDeal;
			}


#pragma region fixed
			// Convert a JSON string to a dictionary value.
			static CefRefPtr<CefDictionaryValue> ParseJSON(const CefString& string) {
				CefRefPtr<CefValue> value = CefParseJSON(string, JSON_PARSER_RFC);
				if (value.get() && value->GetType() == VTYPE_DICTIONARY)
					return value->GetDictionary();
				return NULL;
			}

			// Convert a dictionary value to a JSON string.
			static CefString GetJSON(CefRefPtr<CefDictionaryValue> dictionary) {
				CefRefPtr<CefValue> value = CefValue::Create();
				value->SetDictionary(dictionary);
				return CefWriteJSON(value, JSON_WRITER_DEFAULT);
			}

			// Verify that |key| exists in |dictionary| and has type |value_type|. Fails
			// |callback| and returns false on failure.
			static bool VerifyKey(CefRefPtr<CefDictionaryValue> dictionary,
				const char* key,
				cef_value_type_t value_type,
				CefRefPtr<Callback> callback) {
				if (!dictionary->HasKey(key) || dictionary->GetType(key) != value_type) {
					callback->Failure(
						1,
						"Missing or incorrectly formatted message key: " + std::string(key));
					return false;
				}
				return true;
			}
#pragma endregion fixed
		};


	}  // namespace

	void CreateMessageHandlers(test_runner::MessageHandlerSet& handlers)
	{
		handlers.insert(new Handler());
	}

}
}