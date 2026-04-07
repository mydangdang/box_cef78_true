#include "WebOpt.h"
#include "tests/cefclient/browser/test_runner.h"
#include "tests/cefclient/browser/main_context_impl.h"
#include "tests/cefclient/browser/root_window_win.h"
#include <ShellAPI.h>
#include "tests/shared/browser/util_win.h"
#include "../IniParse.h"

namespace client {
namespace WebOpt {

	namespace {

		const char kNameKey[] = "funcName";

		// Handle messages in the browser process.
		class Handler : public CefMessageRouterBrowserSide::Handler {
		public:
			Handler() {}

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
				if (funcName == "browser_create") {
					bDeal = true;

					resp->SetString(kNameKey, funcName);

					std::string sUrl = request_dict->GetString("url");
					RECT rcPos = { 0 };
					bool bActive = request_dict->GetBool("active");
					bool bMuteOthers = request_dict->GetBool("muteothers");
					bool bShareCache = request_dict->GetBool("sharecache");
					bool bUserIE = request_dict->GetBool("ie");
					int nFixFlash = request_dict->GetInt("fixflash");

					std::string sTag = request_dict->GetString("tag");
					std::string sGroup = request_dict->GetString("group");

					CefRefPtr<CefDictionaryValue> webPos = request_dict->GetDictionary("pos");
					
					if (webPos != NULL) {
						rcPos.left = webPos->GetInt("left");
						rcPos.top = webPos->GetInt("top");
						rcPos.right =  webPos->GetInt("right");
						rcPos.bottom = webPos->GetInt("bottom");

						CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
						if (rootWindow != nullptr) {
							RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
							HWND hParent = browser->GetHost()->GetWindowHandle();
							if(!pRootWindowWin->IsOSR())
							{
								hParent = GetWindow(hParent, GW_CHILD);
								hParent = GetWindow(hParent, GW_CHILD);
							}
							pRootWindowWin->CreateTabBrowser(hParent, rcPos,bUserIE, nFixFlash, bShareCache, bActive, bMuteOthers,sUrl, sTag,sGroup);
						}
						resp->SetString("msg", "creating");
					}
					else {
						resp->SetString("msg", "failed");
					}
				}
				else if (funcName == "tool_create") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					std::string sUrl = request_dict->GetString("url");
					RECT rcPos = { 0 };
					bool bShareCache = request_dict->GetBool("sharecache");
					bool bStandIco = request_dict->GetBool("standico");
					bool bTopMost = request_dict->GetBool("top");
					bool bOSR = request_dict->GetBool("windowless");
					int nAction = 0;
					std::string sAction = request_dict->GetString("killfocusaction");
					if (sAction.find("hide") != -1) nAction = 1;
					else if (sAction.find("close") != -1) nAction = 2;

					std::string sTag = request_dict->GetString("tag");
			

					CefRefPtr<CefDictionaryValue> webPos = request_dict->GetDictionary("pos");
					if (webPos != NULL) {

						CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
						if (rootWindow != nullptr) {

							float fScale = GetWindowScaleFactor(rootWindow->GetWindowHandle());
							rcPos.left = webPos->GetInt("x");
							rcPos.top = webPos->GetInt("y");
							rcPos.right = rcPos.left + webPos->GetInt("width") * fScale;
							rcPos.bottom = rcPos.top + webPos->GetInt("height") * fScale;

							RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
							pRootWindowWin->CreateToolBrowser(rcPos, bOSR,bTopMost, bStandIco, bShareCache, sUrl, sTag, nAction);
						}
						resp->SetString("msg", "creating");
					}
					else {
						resp->SetString("msg", "failed");
					}
				}
				else if (funcName == "browser_close") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					std::string sUniqeTag = request_dict->GetString("tag");
					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(sUniqeTag);
					if (rootWindow != nullptr) {
						RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
						if (pRootWindowWin->CloseTabBrowser(sUniqeTag)) {
							resp->SetString("msg", "succ");
						}
						else {
							resp->SetString("msg", "note exist");
						}
					}
					else {
						resp->SetString("msg", "not a tab browser");
					}
				}
				else if (funcName == "browser_select") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					std::string sUniqeTag = request_dict->GetString("tag");
					bool bMuteOthers = request_dict->GetBool("muteothers");
					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
					if (rootWindow != nullptr) {
						RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
						 
						if (pRootWindowWin->SelectTabBrowser(sUniqeTag, bMuteOthers)) {
							resp->SetString("tag", sUniqeTag);
							resp->SetString("msg", "succ");
						}
						else {
							resp->SetString("msg", "failed");
						}
					}
					else {
						resp->SetString("msg", "failed");
					}
				}
				else if (funcName == "browser_show") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					int nBrowserID = request_dict->GetInt("browserid");
					std::string sUniqeTag = request_dict->GetString("tag");
					bool bShow = request_dict->GetBool("show");

					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
					if (rootWindow != nullptr) {
						RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
						if (nBrowserID > 0) {
							if (pRootWindowWin->ShowTabBrowser(nBrowserID,bShow)) {
								if(bShow)
									resp->SetString("msg", "show");
								else
									resp->SetString("msg", "hide");
							}
							else {
								resp->SetString("msg", "note exist");
							}
						}
						else {
							if (pRootWindowWin->ShowTabBrowser(sUniqeTag, bShow)) {
								resp->SetString("msg", "succ");
							}
							else {
								resp->SetString("msg", "note exist");
							}
						}
					}
					else {
						resp->SetString("msg", "not a tab browser");
					}
				}
				else if (funcName == "browser_hidealltab") {
					bDeal = true;

					resp->SetString(kNameKey, funcName);
					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
					if (rootWindow != nullptr) {
						RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
						pRootWindowWin->HideAllTabBrowser();
						resp->SetString("msg", "succ");
					}
					else {
						resp->SetString("msg", "not a tab browser");
					}
				}
				else if (funcName == "browser_mute") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);

					int nBrowserID = request_dict->GetInt("browserid");
					std::string sUniqeTag = request_dict->GetString("tag");
					bool bMute = request_dict->GetBool("mute");

					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
					if (rootWindow != nullptr) {
						RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
						if (nBrowserID > 0) {
							if (pRootWindowWin->MuteTabBrowser(nBrowserID, bMute)) {
								if (bMute)
									resp->SetString("msg", "muted");
								else
									resp->SetString("msg", "inmuted");
							}
							else {
								resp->SetString("msg", "note exist");
							}
						}
						else {
							if (pRootWindowWin->MuteTabBrowser(sUniqeTag, bMute)) {
								resp->SetString("msg", "succ");
							}
							else {
								resp->SetString("msg", "note exist");
							}
						}
					}
					else {
						resp->SetString("msg", "not a tab browser");
					}
				}
				else if (funcName == "browser_isshow") {
					bDeal = true;

					std::string sUniqeTag = request_dict->GetString("tag");

					resp->SetString(kNameKey, funcName);

					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
					if (rootWindow != nullptr) {
						RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
						bool bIsShow = pRootWindowWin->IsBrowserShow(sUniqeTag);
							resp->SetString("msg", "succ");
							resp->SetString("tag", sUniqeTag);
							resp->SetBool("isshow", bIsShow);
					}
					else {
						resp->SetString("msg", "failed");
					}
				}
				else if (funcName == "browser_ismute") {
					bDeal = true;
					std::string sUniqeTag = request_dict->GetString("tag");
					resp->SetString(kNameKey, funcName);

					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
					if (rootWindow != nullptr) {
						RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
						bool bIsMute = pRootWindowWin->IsBrowserMute(sUniqeTag);
						resp->SetString("msg", "succ");
						resp->SetString("tag", sUniqeTag);
						resp->SetBool("ismute", bIsMute);
					}
					else {
						resp->SetString("msg", "failed");
					}
				}
				else if (funcName == "browser_registObserver") {
					//同时返回自己的browserid;
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					resp->SetString("msg", "succ");
					resp->SetInt("browserid", browser->GetIdentifier());

					std::string szBaseJson = MainContextImpl::Get()->GetBaseInfo();
					CefRefPtr<CefValue> jBase = CefParseJSON(szBaseJson, JSON_PARSER_RFC);
					resp->SetValue("info", jBase);

					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
					if (rootWindow != nullptr) {

						if (request_dict->HasKey("hookopen"))
						{
							bool bHooked = request_dict->GetBool("hookopen");
							rootWindow->SetHookPopupLink(browser->GetIdentifier(),bHooked);
						}

						RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
						pRootWindowWin->SetCallBackJSFunc(browser->GetIdentifier(),callback);

						std::string szClockName = MainContextImpl::Get()->GetClockName();
						if (!szClockName.empty())
							MainContextImpl::Get()->OnClock(szClockName);
					}

					

					RootWindowWin* pRootWnd = (RootWindowWin*)rootWindow.get();
					if (pRootWnd)
					{
						bool bResizable = true;
						if (request_dict->HasKey("resizable")) {
							bResizable = request_dict->GetBool("resizable");
							pRootWnd->EnableResize(bResizable);
						}
						int nMinWidth = 0;
						int nMinHeight = 0;

						if (request_dict->HasKey("minwidth"))
							nMinWidth = request_dict->GetInt("minwidth");
						if (request_dict->HasKey("minheight"))
							nMinHeight = request_dict->GetInt("minheight");

						if(nMinHeight || nMinHeight)
							pRootWnd->SetMinSize(nMinWidth, nMinHeight);
					}
				}
				else if (funcName == "browser_broadcast") {
					bDeal = true;
					int nTargetType = request_dict->GetInt("target");
					resp->SetString(kNameKey, funcName);
					CefRefPtr<CefDictionaryValue> msgObj = request_dict->GetDictionary("msg");

					int nCallerID = browser->GetIdentifier();
					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(nCallerID);
					std::string szCallerTag = rootWindow->GetBrowserTagFromID(nCallerID);

					MainContextImpl::Get()->GetRootWindowManager()->BroadCastMsg(nCallerID, szCallerTag,msgObj, nTargetType);
					resp->SetString("msg", "succ");
				}
				else if (funcName == "browser_reload") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					bool bCache = request_dict->GetBool("cache");
					std::string szTag = request_dict->GetString("tag");
					if (szTag.empty()) {
						if (!bCache) browser->ReloadIgnoreCache();
						else browser->Reload();
					}
					else {
						CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
						if (rootWindow != nullptr) {
							RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
							if (pRootWindowWin->TabReload(szTag, bCache)) {
								resp->SetString("msg", "reload");
							}
							else {
								resp->SetString("msg", "tab browser unexist");
							}
						}
						else {
							resp->SetString("msg", "failed");
						}
					}
					resp->SetString("msg", "reload");
				}
				else if (funcName == "browser_navigate") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					std::string szURL = request_dict->GetString("url");
					bool bBySys = request_dict->GetBool("bysys");
					if (bBySys) {
						ShellExecuteA(NULL, "open", szURL.c_str(), NULL, NULL, SW_SHOWNORMAL);
						resp->SetString("msg", "succ");
					}
					else {
						std::string szTag = request_dict->GetString("tag");

						if (szTag.empty()) {
							browser->GetMainFrame()->LoadURL(szURL);
						}
						else {
							CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
							if (rootWindow != nullptr) {
								RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
								if (pRootWindowWin->TabNavigate(szTag, szURL)) {
									resp->SetString("msg", "succ");
								}
								else {
									resp->SetString("msg", "tab browser unexist");
								}
							}
							else {
								resp->SetString("msg", "failed");
							}
						}
					}
				}
				else if (funcName == "browser_fullscreen") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					std::string szTag = request_dict->GetString("tag");
					
					
					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
					if (rootWindow != nullptr) {
						RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
						if (pRootWindowWin->ShowTopFull(szTag)) {
							resp->SetString("msg", "succ");
						}
						else {
							resp->SetString("msg", "failed");
						}
					}
					else {
						resp->SetString("msg", "failed");
					}
				}
				else if (funcName == "browser_clearcache")
				{
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					std::string szTag = request_dict->GetString("tag");
					
					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
					if (rootWindow != nullptr) {
						RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
						if (pRootWindowWin->ClearCache(szTag)) {
							resp->SetString("msg", "succ");
						}
						else {
							resp->SetString("msg", "tab browser unexist");
						}
					}
					else {
						resp->SetString("msg", "failed");
					}
					
					resp->SetString("msg", "succ");
				}
				else if (funcName == "browser_gettabstates") {
					bDeal = true;
					resp->SetString(kNameKey, funcName);
					std::string szTag = request_dict->GetString("tag");


					CefRefPtr<RootWindow> rootWindow = MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
					if (rootWindow != nullptr) {
						RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
						
						std::vector<tabState> TabStats;
						if (pRootWindowWin->GetTabStates(szTag, TabStats)) {
							
							CefRefPtr<CefListValue> vStats = CefListValue::Create();

							for (UINT i = 0; i < TabStats.size(); i++) {
								CefRefPtr<CefDictionaryValue> state = CefDictionaryValue::Create();
								state->SetInt("browserid", TabStats[i].nBrowserID);
								state->SetString("tag", TabStats[i].szTagName);
								state->SetString("group", TabStats[i].szGroupName);
								state->SetBool("show", TabStats[i].bShow);
								state->SetBool("mute", TabStats[i].bMute);
								state->SetBool("selected", TabStats[i].bSelected);
								state->SetBool("ie", TabStats[i].bIE);
								vStats->SetDictionary(i, state);
							}
							resp->SetList("tabs", vStats);
							resp->SetString("msg", "succ");
						}
						else {
							resp->SetString("msg", "failed");
						}
					}
					else {
						resp->SetString("msg", "failed");
					}
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