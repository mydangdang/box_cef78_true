// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "tests/cefclient/browser/root_window_win.h"

#include <shellscalingapi.h>

#include "include/base/cef_bind.h"
#include "include/base/cef_build.h"
#include "include/cef_app.h"
#include "tests/cefclient/browser/browser_window_osr_win.h"
#include "tests/cefclient/browser/browser_window_std_win.h"
#include "tests/cefclient/browser/main_context.h"
#include "tests/cefclient/browser/resource.h"
#include "tests/cefclient/browser/temp_window.h"
#include "tests/shared/browser/geometry_util.h"
#include "tests/shared/browser/main_message_loop.h"
#include "tests/shared/browser/util_win.h"
#include "tests/shared/common/client_switches.h"
#include "main_context_impl.h"
#include "include/cef_path_util.h"
#include "../RTCTX.h"
#include "../WinMsg.h"

#define MAX_URL_LENGTH 255
#define BUTTON_WIDTH 72
#define URLBAR_HEIGHT 24

namespace client {

namespace {

int GetButtonWidth(HWND hwnd) {
  return LogicalToDevice(BUTTON_WIDTH, GetWindowScaleFactor(hwnd));
}

int GetURLBarHeight(HWND hwnd) {
  return LogicalToDevice(URLBAR_HEIGHT, GetWindowScaleFactor(hwnd));
}

}  // namespace

RootWindowWin::RootWindowWin()
    : with_controls_(false),
      always_on_top_(false),
      with_osr_(false),
      isDevsTool(false),
      with_extension_(false),
      is_popup_(false),
      start_rect_(),
      initialized_(false),
      hwnd_(NULL),
      draggable_region_(NULL),
      font_(NULL),
      font_height_(0),
      back_hwnd_(NULL),
      forward_hwnd_(NULL),
      reload_hwnd_(NULL),
      stop_hwnd_(NULL),
      edit_hwnd_(NULL),
      edit_wndproc_old_(NULL),
      find_hwnd_(NULL),
      find_message_id_(0),
      find_wndproc_old_(NULL),
      find_state_(),
      find_next_(false),
      find_match_case_last_(false),
      window_destroyed_(false),
      browser_destroyed_(false),
      called_enable_non_client_dpi_scaling_(false) {
    nMainBrowserID = 0;
  find_buff_[0] = 0;
  lastWndState = LWS_NOR;
  bIsBossIn = false;
  isAsTool = false;
  isToolWnd = false;
  m_bResizable = true;
  m_bNeedResotreForBossKey = false;
  m_sMinSize.cx = m_sMinSize.cy = 0;
  m_nKillFocusAction = 0;
  hCurForgroundWnd = NULL;
  // Create a HRGN representing the draggable window area.
  draggable_region_ = ::CreateRectRgn(0, 0, 0, 0);
}

RootWindowWin::~RootWindowWin() {
  REQUIRE_MAIN_THREAD();

  ::DeleteObject(draggable_region_);
  ::DeleteObject(font_);

  // The window and browser should already have been destroyed.
  DCHECK(window_destroyed_);
  DCHECK(browser_destroyed_);
}

void RootWindowWin::Init(RootWindow::Delegate* delegate,
                         const RootWindowConfig& config,
                         const CefBrowserSettings& settings) {
  DCHECK(delegate);
  DCHECK(!initialized_);

  delegate_ = delegate;
  with_controls_ = config.with_controls;
  always_on_top_ = config.always_on_top;
  with_osr_ = config.with_osr;
  with_extension_ = config.with_extension;

  start_rect_.left = config.bounds.x;
  start_rect_.top = config.bounds.y;
  start_rect_.right = config.bounds.x + config.bounds.width;
  start_rect_.bottom = config.bounds.y + config.bounds.height;

  CreateBrowserWindow(config.url);

  initialized_ = true;

  // Create the native root window on the main thread.
  if (CURRENTLY_ON_MAIN_THREAD()) {
    CreateRootWindow(settings, config.initially_hidden);
  } else {
    MAIN_POST_CLOSURE(base::Bind(&RootWindowWin::CreateRootWindow, this,
                                 settings, config.initially_hidden));
  }
}

void RootWindowWin::InitAsPopup(RootWindow::Delegate* delegate,
                                bool with_controls,
                                bool with_osr,
                                const CefPopupFeatures& popupFeatures,
                                CefWindowInfo& windowInfo,
                                CefRefPtr<CefClient>& client,
                                CefBrowserSettings& settings) {
  CEF_REQUIRE_UI_THREAD();

  DCHECK(delegate);
  DCHECK(!initialized_);

  //alway false;

  with_controls_ = false;
  delegate_ = delegate;
  isDevsTool = with_controls;
  with_osr_ = with_osr;
  is_popup_ = true;

  if(isDevsTool)
  {
	  if (popupFeatures.xSet)
		  start_rect_.left = popupFeatures.xSet;

	  if (popupFeatures.ySet)
		  start_rect_.top = popupFeatures.ySet;
	  if (popupFeatures.widthSet)
		  start_rect_.right = start_rect_.left + popupFeatures.widthSet;
	  if (popupFeatures.heightSet)
		  start_rect_.bottom = start_rect_.top + popupFeatures.heightSet;
  }
  else {
      start_rect_.left = windowInfo.x;
      start_rect_.top = windowInfo.y;
      start_rect_.right = start_rect_.left + windowInfo.width;
      start_rect_.bottom = start_rect_.top + windowInfo.height;
  }

  CreateBrowserWindow(std::string());

  initialized_ = true;

  // The new popup is initially parented to a temporary window. The native root
  // window will be created after the browser is created and the popup window
  // will be re-parented to it at that time.
  browser_window_->GetPopupConfig(TempWindow::GetWindowHandle(), windowInfo,
                                  client, settings);
}

void RootWindowWin::InitAsTool(RootWindow::Delegate* delegate, 
    bool bOSR,
    bool bTopMost,
    bool bToolWnd,
	const std::string& url,
	const std::string& sTag, RECT rcPos, int nAction,
	CefRefPtr<CefRequestContext> ctx)
{
	DCHECK(delegate);
	DCHECK(!initialized_);

    isAsTool = true;
	delegate_ = delegate;
	with_controls_ = false;
	always_on_top_ = bTopMost;
	with_osr_ = bOSR;
	with_extension_ = false;
    start_rect_ = rcPos;
    isToolWnd = bToolWnd;
    m_nKillFocusAction = nAction;
    szMainBrowserTag = sTag;

	CreateBrowserWindow(url);

    if (browser_window_) browser_window_->SetUniqeTag(szMainBrowserTag);

	initialized_ = true;
    CefBrowserSettings settings;
    MainContext::Get()->PopulateBrowserSettings(&settings);
	// Create the native root window on the main thread.
	if (CURRENTLY_ON_MAIN_THREAD()) {
		CreateRootWindow(settings, true);
	}
	else {
		MAIN_POST_CLOSURE(base::Bind(&RootWindowWin::CreateRootWindow, this,settings, true));
	}
}

void RootWindowWin::Show(ShowMode mode) {
  REQUIRE_MAIN_THREAD();

  if (!hwnd_)
    return;

  int nCmdShow = SW_SHOWNORMAL;
  switch (mode) {
    case ShowMinimized:
      nCmdShow = SW_SHOWMINIMIZED;
      break;
    case ShowMaximized:
      nCmdShow = SW_SHOWMAXIMIZED;
      break;
    case ShowNoActivate:
      nCmdShow = SW_SHOWNOACTIVATE;
      break;
    case ShowForeground:
    {
		if (!IsIconic(hwnd_))
            nCmdShow = SW_SHOWNOACTIVATE;
        if(!IsWindowVisible(hwnd_))
            nCmdShow = SW_SHOWNOACTIVATE;
        SetForegroundWindow(hwnd_);
    }
    break;
    default:
      break;
  }

  ShowWindow(hwnd_, nCmdShow);
  UpdateWindow(hwnd_);
}

void RootWindowWin::Hide() {
  REQUIRE_MAIN_THREAD();

  if (hwnd_)
    ShowWindow(hwnd_, SW_HIDE);
}

void RootWindowWin::SetBounds(int x, int y, size_t width, size_t height) {
  REQUIRE_MAIN_THREAD();

  if (hwnd_) {
    SetWindowPos(hwnd_, NULL, x, y, static_cast<int>(width),
                 static_cast<int>(height), SWP_NOZORDER);
  }
}

void RootWindowWin::Close(bool force) {
  REQUIRE_MAIN_THREAD();

  if (hwnd_) {
    if (force)
      DestroyWindow(hwnd_);
	else
	{
		if(browser_window_ && browser_window_->GetBrowser())
			PostMessage(hwnd_, WM_CLOSE, browser_window_->GetBrowser()->GetIdentifier(), 0);
		else
			PostMessage(hwnd_, WM_CLOSE, 0, 0);
	}
  }
}

void RootWindowWin::SetDeviceScaleFactor(float device_scale_factor) {
  REQUIRE_MAIN_THREAD();

  if (browser_window_ && with_osr_)
    browser_window_->SetDeviceScaleFactor(device_scale_factor);
}

float RootWindowWin::GetDeviceScaleFactor() const {
  REQUIRE_MAIN_THREAD();

  if (browser_window_ && with_osr_)
    return browser_window_->GetDeviceScaleFactor();

  NOTREACHED();
  return 0.0f;
}

CefRefPtr<CefBrowser> RootWindowWin::GetBrowser() const {
  REQUIRE_MAIN_THREAD();

  if (browser_window_)
    return browser_window_->GetBrowser();
  return NULL;
}

ClientWindowHandle RootWindowWin::GetWindowHandle() const {
  REQUIRE_MAIN_THREAD();
  return hwnd_;
}

bool RootWindowWin::WithWindowlessRendering() const {
  REQUIRE_MAIN_THREAD();
  return with_osr_;
}

bool RootWindowWin::WithExtension() const {
  REQUIRE_MAIN_THREAD();
  return with_extension_;
}

void RootWindowWin::BroadCastMsg(int nCallerID, const std::string& sCallerTag, CefRefPtr<CefDictionaryValue> msgJson, int nTargetType)
{
    REQUIRE_MAIN_THREAD();
	CefRefPtr<CefDictionaryValue> resp = CefDictionaryValue::Create();
	resp->SetString("name", "onBroadCast");
	resp->SetInt("senderid", nCallerID);
    resp->SetString("sendertag", sCallerTag);


// 	CefRefPtr<CefValue> testValue = CefValue::Create();
//     testValue->SetDictionary(msgJson);
//     CefString szMsgJson = CefWriteJSON(testValue, JSON_WRITER_DEFAULT);
//     OutputDebugStringA("\n=========U===========\n");
//     std::string szText = szMsgJson.ToString();
//     OutputDebugStringA((LPCSTR)szText.c_str());
//     OutputDebugStringA("\n=========D===========\n");
// 	if (szText.empty())
// 	{
// 		return;
// 	}

	resp->SetDictionary("msg", msgJson);
	CefRefPtr<CefValue> value = CefValue::Create();
	value->SetDictionary(resp);
	CefString szResp = CefWriteJSON(value, JSON_WRITER_DEFAULT);

    if(nCallerID != browser_window_->GetBrowserID())
	{
		CefRefPtr<CefMessageRouterBrowserSide::Callback> fCB = browser_window_->GetObserFunc();
		if (fCB) fCB->Success(szResp);
	}

    if (nTargetType == 2) return;

	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) 
    {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetBrowserID() != nCallerID) 
        {
			CefRefPtr<CefMessageRouterBrowserSide::Callback> fCB = pBrowserWindow->GetObserFunc();
			if (fCB) fCB->Success(szResp);
            else
            {
                if (pBrowserWindow->IsIEKernel() && pBrowserWindow->GetIEHandler())
                {
                    pBrowserWindow->GetIEHandler()->OnRecvBroadCast(szResp);
                }
            }
		}
	}
}


void RootWindowWin::PostCustomMsg(int nCallerID, std::string& sTagName, const std::string& sMsgName, CefRefPtr<CefDictionaryValue> msgJson)
{
	CefRefPtr<CefMessageRouterBrowserSide::Callback> fCB = browser_window_->GetObserFunc();
	if (fCB)
	{
		CefRefPtr<CefDictionaryValue> resp = CefDictionaryValue::Create();
		resp->SetString("name", sMsgName);
		resp->SetDictionary("arg", msgJson);
		CefRefPtr<CefValue> value = CefValue::Create();
		value->SetDictionary(resp);
		CefString szResp = CefWriteJSON(value, JSON_WRITER_DEFAULT);
		fCB->Success(szResp);
	}
}

bool RootWindowWin::OwnerBrowser(int nBrowserID)
{
    if (nBrowserID == nMainBrowserID || (GetBrowser() && nBrowserID == GetBrowser()->GetIdentifier()))
        return true;
    else {
		std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
		for (; it != vChildBrowsers.end(); it++) {
			BrowserWindow* pBrowserWindow = *it;
			if (pBrowserWindow->GetBrowserID() == nBrowserID) {
                return true;
			}
		}

    }
    return false;
}


bool RootWindowWin::OwnerBrowser(const std::string& sTagName)
{
	if (sTagName == browser_window_->GetUniqeTag())
		return true;
	else {
		std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
		for (; it != vChildBrowsers.end(); it++) {
			BrowserWindow* pBrowserWindow = *it;
			if (pBrowserWindow->GetUniqeTag() == sTagName) {
				return true;
			}
		}

	}
	return false;
}

CefRefPtr<CefBrowser> RootWindowWin::GetChildBrowser(int nBrowserID)
{
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetBrowserID() == nBrowserID) {
			return pBrowserWindow->GetBrowser();
		}
	}
	return nullptr;
}


CefRefPtr<CefBrowser> RootWindowWin::GetChildBrowser(const std::string& sTag)
{
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetUniqeTag() == sTag) {
			return pBrowserWindow->GetBrowser();
		}
	}
	return nullptr;
}

bool RootWindowWin::CreateTabBrowser(HWND hParent, RECT rcPos, bool bIE, int nFixFlash,bool bShareCache , bool bActive, bool bMuteOther, const std::string& url, const std::string& tag, const std::string& group)
{
    std::string szURL = url;
	if (!szURL.empty() && szURL.find("http") == -1 && szURL.find("www") == -1 && szURL.find("com") == -1 && szURL.find(":") == -1)
	{
        szURL = RTCTX::CurPath() + "/" + szURL;
	}
    

	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
        if (pBrowserWindow->GetUniqeTag() == tag) {
            SelectTabBrowser(tag, bMuteOther);
            return true;
        }
	}

    BrowserWindow* tabBrowserWindow = new BrowserWindowStdWin(this, szURL,tag,group,bIE,nFixFlash,true);
    if(!bActive) tabBrowserWindow->SetSelected(false);
    vChildBrowsers.push_back(tabBrowserWindow);

    tabBrowserWindow->SetMarginRect(rcPos);
    if(!group.empty())
	{
        //mute hide group
		it = vChildBrowsers.begin();
		for (; it != vChildBrowsers.end(); it++) {
			BrowserWindow* pBrowserWindow = *it;
			if (pBrowserWindow->GetBrowserGroup() == group && pBrowserWindow->GetUniqeTag() != tag && pBrowserWindow->IsSelected()) {
                pBrowserWindow->TabHide();
                if (bMuteOther) {
                    pBrowserWindow->SetMute(true);
                }
                if (bActive) {
                    pBrowserWindow->SetSelected(false);
                    mGroupLastTag[pBrowserWindow->GetBrowserGroup()] = pBrowserWindow->GetUniqeTag();
                }
			}
		}
	}

// 	HINSTANCE hInstance = GetModuleHandle(NULL);
// 	WNDCLASS wc = { 0 };
// 	wc.cbClsExtra = wc.cbWndExtra = 0;
// 	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
// 	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
// 	wc.hIcon = NULL;
// 	wc.hInstance = hInstance;
// 	wc.lpfnWndProc = ToolWndProc;
// 	wc.lpszClassName = L"ToolTopWnd";
// 	wc.lpszMenuName = NULL;
// 	wc.style = 0;
// 	RegisterClass(&wc);
// 
// 	HWND hTopWnd = CreateWindowEx(0, L"ToolTopWnd", L"ToolTopWnd",
// 		WS_CHILDWINDOW | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE,
// 		0, 0, 800, 600, NULL, NULL, hInstance, this);
// 	CefRect cef_rect(0, 0, 800, 600);
// 	hParent = hTopWnd;
// 	CreateRootWindow(CefBrowserSettings(), false);

    CefRect cef_rect(rcPos.left, rcPos.top, rcPos.right - rcPos.left, rcPos.bottom - rcPos.top);
	RECT rcParent = { 0 };
	GetWindowRect(hParent, &rcParent);
    float fScal = GetWindowScaleFactor(hwnd_);

	cef_rect.x = rcPos.left * fScal;
	cef_rect.y = rcPos.top * fScal;
	cef_rect.width = rcParent.right - rcParent.left - (rcPos.left + rcPos.right)* fScal;
	cef_rect.height = rcParent.bottom - rcParent.top - (rcPos.top + rcPos.bottom)*fScal;


    //复用 父窗口的 reqCtx 
    //若不想公用 cache 可以这里创建一个新的rc auto reqCtx = delegate_->GetRequestContext(this);
    CefRefPtr<CefRequestContext> reqCtx = delegate_->GetRequestContext(this,!bShareCache);
    tabBrowserWindow->CreateBrowser(hParent, cef_rect, browser_settings_,NULL, reqCtx);
    
    return true;
}


bool RootWindowWin::CreateToolBrowser(RECT rcPos, bool bOSR, bool bTopMost, bool bStandIco, bool bShareCache, const std::string& url, const std::string& sTag, int nKillFocusAction)
{
//     这里创建一个 tempwindow作为parent
//     等接受到browsercreated后 createrootwindow
//     oncreate里面把tabBrowserWindow showpopup 绑定过去;
//     整个流程与 createpopupbrowser一致
//     不同的是, window.open 响应的 onbeforepopup 是在 获取了tempwindow, cefclient后  chrome内部通过 host::creatbrowser创建完窗口
//     然后onaftercreate  createrootwindow  然后oncreate绑定
//     所以我们模拟 window.open则需要  tempwindow 然后 hostcreatebrowser 然后 onaftercreate createrootwindow再oncreate绑定
//     注意 这里也要控制 requectcontext
//     单独写一个js函数来测试
	// 独立一个js函数   测试互通数据 cache
    // https://bitbucket.org/chromiumembedded/cef/issues/2969
	std::string szURL = url;
	if (!szURL.empty() && szURL.find("http") == -1 && szURL.find("www") == -1 && szURL.find("com") == -1 && szURL.find(":"))
	{
		szURL = RTCTX::CurPath() + "/" + szURL;
	}

    if (MainContext::Get()->GetRootWindowManager()->HasRootWindowAsTool(sTag))
    {
        CefRefPtr<RootWindow> rootWindow = MainContext::Get()->GetRootWindowManager()->GetWindowForBrowser(sTag);
        if (rootWindow != nullptr) {
            RootWindowWin* pRootWindowWin = (RootWindowWin*)rootWindow.get();
			HWND hRootWnd = pRootWindowWin->GetWindowHandle();
            

			RECT rcOldPos = {0};
			GetWindowRect(hRootWnd, &rcOldPos);
			if (rcOldPos.left != rcPos.left || rcOldPos.top != rcPos.top || rcOldPos.right != rcPos.right || rcOldPos.bottom != rcPos.bottom)
				SetWindowPos(hRootWnd, NULL, rcPos.left, rcPos.top, rcPos.right - rcPos.left, rcPos.bottom - rcPos.top, SWP_NOZORDER | SWP_SHOWWINDOW);
			pRootWindowWin->Show(RootWindow::ShowForeground);
            return true;
        }
    }

	CefRefPtr<CefRequestContext> reqCtx = delegate_->GetRequestContext(this, !bShareCache);
	MainContext::Get()->GetRootWindowManager()->CreateRootWindowAsTool(bOSR,bTopMost,!bStandIco, szURL, sTag,rcPos,nKillFocusAction, reqCtx);

	return true;
}

void RootWindowWin::HideAllTabBrowser()
{
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		pBrowserWindow->TabHide();
		pBrowserWindow->SetSelected(false);
	}
  
	if (hwnd_)
		SetWindowText(hwnd_, CefString(browser_window_->GetCurTitle()).ToWString().c_str());
}

bool RootWindowWin::ShowTabBrowser(int nBrowserID, bool bShow)
{
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetBrowserID() == nBrowserID)
		{
			if (bShow) pBrowserWindow->TabShow();
			else pBrowserWindow->TabHide();
			return true;
		}
	}
	return false;
}


bool RootWindowWin::ShowTabBrowser(const std::string& sUniqueTag, bool bShow)
{
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetUniqeTag() == sUniqueTag)
		{
            if (bShow) pBrowserWindow->TabShow();
            else pBrowserWindow->TabHide();
			return true;
		}
	}
	return false;
}


bool RootWindowWin::MuteTabBrowser(int nBrowserID, bool bMute)
{
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetBrowserID() == nBrowserID)
		{
			if (pBrowserWindow->IsIEKernel())
			{
				if(pBrowserWindow->GetIEHandler())
					pBrowserWindow->GetIEHandler()->SetMute(bMute);
			}
			else
				pBrowserWindow->GetBrowser()->GetHost()->SetAudioMuted(bMute);
			return true;
		}
	}
	return false;
}


bool RootWindowWin::MuteTabBrowser(const std::string& sUniqueTag, bool bMute)
{
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetUniqeTag() == sUniqueTag)
		{
            if (pBrowserWindow->IsIEKernel())
                pBrowserWindow->GetIEHandler()->SetMute(bMute);
            else
                pBrowserWindow->GetBrowser()->GetHost()->SetAudioMuted(bMute);
			return true;
		}
	}
	return false;
}

bool RootWindowWin::CloseTabBrowser(int nBrowserID)
{
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;

		CefRefPtr<CefBrowser> cefBrowser = pBrowserWindow->GetBrowser();
        std::string sGourp = pBrowserWindow->GetBrowserGroup();
		if (nBrowserID == pBrowserWindow->GetBrowserID()) {
            return pBrowserWindow->CloseBrowser();
		}
	}
    return false;
}


bool RootWindowWin::CloseTabBrowser(const std::string& sUniqueTag)
{
	if (sUniqueTag == browser_window_->GetUniqeTag()) {
		Close(false);
		return true;
	}

	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;

		if (sUniqueTag == pBrowserWindow->GetUniqeTag()) {
			bool bSelected = pBrowserWindow->IsSelected();
			pBrowserWindow->CloseBrowser();
			if(bSelected)
			{
				std::string sNextSelect = mGroupLastTag[pBrowserWindow->GetBrowserGroup()];
				SelectTabBrowser(sNextSelect, true);
			}
			return true;
		}
		
	}
	return false;
}


bool RootWindowWin::SelectTabBrowser(const std::string& sUniqueTag, bool bMuteOthers)
{
    bool bFound = false;
    std::string sGroup;
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetUniqeTag() == sUniqueTag) {
            bFound = true;
            sGroup = pBrowserWindow->GetBrowserGroup();
            break;
		}
	}

    if (!sGroup.empty()) {
		it = vChildBrowsers.begin();
		for (; it != vChildBrowsers.end(); it++) {
			BrowserWindow* pBrowserWindow = *it;
			if (pBrowserWindow->GetBrowserGroup() == sGroup) {
                if (pBrowserWindow->GetUniqeTag() == sUniqueTag) {
                    pBrowserWindow->SetSelected(true);
                    pBrowserWindow->TabShow();
					OnSize(false);
                }
                else {
                    mGroupLastTag[pBrowserWindow->GetBrowserGroup()] = pBrowserWindow->GetUniqeTag();
                    pBrowserWindow->SetSelected(false);
                    pBrowserWindow->TabHide();
                    if(bMuteOthers) pBrowserWindow->SetMute(true);
                }
			}
		}
    }
    return bFound;
}


bool RootWindowWin::TabNavigate(const std::string& sUniqueTag, const std::string& sURL)
{
	if (sUniqueTag == browser_window_->GetUniqeTag()) {
		browser_window_->GetBrowser()->GetMainFrame()->LoadURL(sURL);
		return true;
	}

    bool bFound = false;
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetUniqeTag() == sUniqueTag) {
            if(!pBrowserWindow->IsIEKernel())
                pBrowserWindow->GetBrowser()->GetMainFrame()->LoadURL(sURL);
            else
            {
				if(pBrowserWindow->GetIEHandler()) pBrowserWindow->GetIEHandler()->NavTo(sURL);
            }
            bFound = true;
			break;
		}
	}
    return bFound;
}


bool RootWindowWin::TabReload(const std::string& sUniqueTag, bool bCache)
{
	if (sUniqueTag == browser_window_->GetUniqeTag()) {
		if (bCache)
			browser_window_->GetBrowser()->Reload();
		else
			browser_window_->GetBrowser()->ReloadIgnoreCache();
		return true;
	}

	bool bFound = false;
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetUniqeTag() == sUniqueTag) {
			if (!pBrowserWindow->IsIEKernel())
			{
				if (bCache)
					pBrowserWindow->GetBrowser()->Reload();
				else
					pBrowserWindow->GetBrowser()->ReloadIgnoreCache();
			}
			else
			{
				if (pBrowserWindow->GetIEHandler()) pBrowserWindow->GetIEHandler()->Reload(bCache);
			}
			bFound = true;
			break;
		}
	}
	return bFound;
}

class CacheClearedCB : public CefDeleteCookiesCallback {
public:
    void OnComplete(int num_deleted)
    {
//         if (browser.get())
//         {
//             browser->ReloadIgnoreCache();
//         }
    }
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(CacheClearedCB);
};

class CookiePeek : public CefCookieVisitor {
public:
    bool Visit(const CefCookie& cookie,int count,int total,bool& deleteCookie)
    {
        deleteCookie = true;
        return true;
    }
    IMPLEMENT_REFCOUNTING(CookiePeek);
};

bool RootWindowWin::ClearCache(const std::string& sUniqueTag)
{
	bool bFound = false;
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetUniqeTag() == sUniqueTag) {
			if (pBrowserWindow->IsIEKernel())
			{
				if(pBrowserWindow->GetIEHandler()) pBrowserWindow->GetIEHandler()->ClearCache();
			}
            else
            {
                CefRefPtr<CacheClearedCB> cb = new CacheClearedCB();
                cb->browser = pBrowserWindow->GetBrowser();
				CefRefPtr<CefCookieManager> manager = CefCookieManager::GetGlobalManager(NULL);
                std::string szURL = pBrowserWindow->GetBrowser()->GetMainFrame()->GetURL();

                CefRefPtr<CookiePeek> peeker = new CookiePeek();
                manager->VisitUrlCookies(szURL, true, peeker);
                manager->DeleteCookies(szURL, "", cb);
            }
			bFound = true;
			break;
		}
	}
	return bFound;
}

bool RootWindowWin::ShowTopFull(const std::string& sUniqueTag)
{
	bool bFound = false;
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetUniqeTag() == sUniqueTag) {
            bFound = true;
			m_FullTopManager.SetWndFullTop((BrowserWindowStdWin*)pBrowserWindow, hwnd_,GetWindowScaleFactor(hwnd_));
			break;
		}
	}
	return bFound;
}

bool RootWindowWin::GetTabStates(const std::string& sTagName, std::vector<tabState>& stats)
{
	bool bGetAll = false;
	bool bFound = false;
	if (sTagName.empty())
	{
		bGetAll = true;
		bFound = true;
	}
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetUniqeTag() == sTagName || bGetAll) {
			tabState state;
			state.nBrowserID = pBrowserWindow->GetBrowserID();
			state.szTagName = pBrowserWindow->GetUniqeTag();
			state.szGroupName = pBrowserWindow->GetBrowserGroup();
			state.bIE = pBrowserWindow->IsIEKernel();
			state.bSelected = pBrowserWindow->IsSelected();
			state.bShow = pBrowserWindow->IsShow();
			state.bMute = pBrowserWindow->IsMute();
			stats.push_back(state);
			if (!bGetAll)
			{
				bFound = true;
				break;
			}
		}
	}
	return bFound;
}

bool RootWindowWin::IsBrowserShow(const std::string& sTagName)
{
	bool bFound = false;
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetUniqeTag() == sTagName) {
            bFound = true;
			return pBrowserWindow->IsShow();
		}
	}
	return bFound;
}

bool RootWindowWin::IsBrowserMute(const std::string& sTagName)
{
	bool bFound = false;
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->GetUniqeTag() == sTagName) {
            bFound = true;
			return pBrowserWindow->IsMute();
		}
	}
	return bFound;
}

void RootWindowWin::OnTopFullHotKey()
{
    if (m_FullTopManager.IsFullScreen()) m_FullTopManager.RestoreWnd();
}

void RootWindowWin::OnBossHotKey()
{
    if (bIsBossIn)
    {
        //恢复
        bIsBossIn = false;
		std::vector<BrowserWindow*>::iterator it = vBossHushedBrowser.begin();
		for (; it != vBossHushedBrowser.end(); it++) {
			BrowserWindow* pBrowserWindow = *it;
            pBrowserWindow->SetMute(false);
            if (pBrowserWindow->IsSelected() )
                pBrowserWindow->TabShow();
		}
        vBossHushedBrowser.clear();
        if (m_bNeedResotreForBossKey)
        {
            ShowWindow(hwnd_, SW_SHOW);
        }
        if (m_FullTopManager.IsFullScreen()) m_FullTopManager.Show();
    }
    else
    {
        //隐藏
        bIsBossIn = true;
		std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
		for (; it != vChildBrowsers.end(); it++) {
			BrowserWindow* pBrowserWindow = *it;
			if (!pBrowserWindow->IsMute()) {
                vBossHushedBrowser.push_back(pBrowserWindow);
                pBrowserWindow->SetMute(true);
			}
			if ( pBrowserWindow->IsSelected())
				pBrowserWindow->TabHide();
		}

        if (IsWindowVisible(hwnd_))
        {
            ShowWindow(hwnd_, SW_HIDE);
            m_bNeedResotreForBossKey = true;
        }
        else
        {
            m_bNeedResotreForBossKey = false;
        }

        if (m_FullTopManager.IsFullScreen()) m_FullTopManager.Hide();
    }
}

bool RootWindowWin::IsToolWnd()
{
    return isAsTool;
}

bool RootWindowWin::IsOSR()
{
    return with_osr_;
}

void RootWindowWin::SetHookPopupLink(int nID, bool bHook)
{
	if (nID == nMainBrowserID) {
        m_bHookPopupLink = bHook;
	}
	else {
		std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
		for (; it != vChildBrowsers.end(); it++) {
			BrowserWindow* pBrowserWindow = *it;
			if (pBrowserWindow->GetBrowserID() == nID) {
				pBrowserWindow->SetHookPopupLink(bHook);
				break;
			}
		}
	}
}

bool RootWindowWin::IsHookPopupLink(int nID)
{
	if (nID == nMainBrowserID) {
        return m_bHookPopupLink;
	}
	else {
		std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
		for (; it != vChildBrowsers.end(); it++) {
			BrowserWindow* pBrowserWindow = *it;
			if (pBrowserWindow->GetBrowserID() == nID) {
				return pBrowserWindow->IsHookPopupLink();
			}
		}
        return false;
	}
}

void RootWindowWin::OnToastEnd(int nCallerId, std::wstring szMsg)
{
	CefRefPtr<CefDictionaryValue> resp = CefDictionaryValue::Create();
	resp->SetString("name", "onToastEnd");
	resp->SetString("tag", GetBrowserTagFromID(nCallerId));
	resp->SetString("msg", szMsg.c_str());

	CefRefPtr<CefValue> value = CefValue::Create();
	value->SetDictionary(resp);
	CefString szResp = CefWriteJSON(value, JSON_WRITER_DEFAULT);

	if (browser_window_)
	{
		CefRefPtr<CefMessageRouterBrowserSide::Callback> fCB = browser_window_->GetObserFunc();
		if (fCB) fCB->Success(szResp);
	}

}

void RootWindowWin::SetCallBackJSFunc(int nBrowserID,CefRefPtr<CefMessageRouterBrowserSide::Callback> callBackFunc)
{
    if (nBrowserID == nMainBrowserID) {
        browser_window_->SetJSObserFunc(callBackFunc);
    }
    else {
		std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
        for (; it != vChildBrowsers.end(); it++) {
            BrowserWindow* pBrowserWindow = *it;
            if (pBrowserWindow->GetBrowserID() == nBrowserID) {
                pBrowserWindow->SetJSObserFunc(callBackFunc);
                break;
            }
        }
    }
}

void RootWindowWin::EnableResize(bool bEnable)
{
	if (m_bResizable != bEnable) {
		DWORD dwStyle = GetWindowLongPtr(hwnd_, GWL_STYLE);
		if (bEnable) {
			dwStyle |= WS_SYSMENU;
			dwStyle |= WS_MINIMIZEBOX;
			dwStyle |= WS_MAXIMIZEBOX;
		}
		else {
			dwStyle &= ~WS_SYSMENU;
			dwStyle &= ~WS_MINIMIZEBOX;
			dwStyle &= ~WS_MAXIMIZEBOX;
		}
		SetWindowLongPtr(hwnd_, GWL_STYLE, dwStyle);
		SetWindowPos(hwnd_, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
		m_bResizable = bEnable;
	}
}

void RootWindowWin::SetMinSize(int nMinWidth, int nMinHeight)
{
    m_sMinSize.cx = nMinWidth;
    m_sMinSize.cy = nMinHeight;
}

std::string RootWindowWin::GetBrowserTagFromID(int nBrowserID)
{
    if (browser_window_ && browser_window_->GetBrowserID() == nBrowserID)
    {
        return browser_window_->GetUniqeTag();
    }
    else
    {
		std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
		for (; it != vChildBrowsers.end(); it++) {
			BrowserWindow* pBrowserWindow = *it;
			if (pBrowserWindow->GetBrowserID() == nBrowserID ) 
            {
                return pBrowserWindow->GetUniqeTag();
			}
		}
        return "";
    }
	
}

int RootWindowWin::GetMainBrowserID()
{
    return nMainBrowserID;
}

std::string RootWindowWin::GetMainBrowserTag()
{
    return szMainBrowserTag;
}

void RootWindowWin::ShowToast(int nCallerID,const std::wstring& szMsg, RECT rcPos, int nShowDuration, int nFadeDuration)
{
    m_ToastWnd.SetToastInfo(this,nCallerID,szMsg, nShowDuration, nFadeDuration);
    m_ToastWnd.CreateLayeredWnd(hwnd_, rcPos);
    m_ToastWnd.SetVisible(true);
}

bool RootWindowWin::IsUnderBoss()
{
    return bIsBossIn;
}

void RootWindowWin::CreateBrowserWindow(const std::string& startup_url) {
  if (with_osr_) {
    OsrRendererSettings settings = {};
    MainContext::Get()->PopulateOsrSettings(&settings);
    browser_window_.reset(new BrowserWindowOsrWin(this, startup_url, settings));
  } else {
    browser_window_.reset(new BrowserWindowStdWin(this, startup_url,"","",false));
  }
}

void RootWindowWin::CreateRootWindow(const CefBrowserSettings& settings,
                                     bool initially_hidden) {
  REQUIRE_MAIN_THREAD();
  DCHECK(!hwnd_);

  HINSTANCE hInstance = GetModuleHandle(NULL);

  // Load strings from the resource file.
  std::wstring& window_title = GetResourceString(IDS_APP_TITLE);
  std::wstring& window_class = GetResourceString(IDC_CEFCLIENT);

  const cef_color_t background_color = MainContext::Get()->GetBackgroundColor();
  const HBRUSH background_brush = CreateSolidBrush(
      RGB(CefColorGetR(background_color), CefColorGetG(background_color),
          CefColorGetB(background_color)));

  if (with_osr_)
  {
      window_class = window_class + L"osr";
	  static bool osrclass_registered = false;
	  if (!osrclass_registered)
	  {
          osrclass_registered = true;
		  RegisterRootClass(hInstance, window_class, background_brush);
	  }
  }
  else
  {
	  static bool class_registered = false;
	  if (!class_registered)
	  {
		  class_registered = true;
		  RegisterRootClass(hInstance, window_class, background_brush);
	  }
  }
  // Register the message used with the find dialog.
  find_message_id_ = RegisterWindowMessage(FINDMSGSTRING);
  CHECK(find_message_id_);

  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();
  const bool no_activate = command_line->HasSwitch(switches::kNoActivate);

  DWORD dwStyle = 0;
  if (!isDevsTool) //去掉 WS_CAPTIONd 有动画与 IE内嵌不连贯
  {
      dwStyle = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      if (m_bResizable) {
          dwStyle |= WS_MINIMIZEBOX;
          dwStyle |= WS_MAXIMIZEBOX;
      }
  }
  else
      dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_EX_TOPMOST;

  DWORD dwExStyle = always_on_top_ ? WS_EX_TOPMOST : 0;
  if (no_activate) {
    // Don't activate the browser window on creation.
    dwExStyle |= WS_EX_NOACTIVATE;
  }
  if (isToolWnd)
      dwExStyle |= WS_EX_TOOLWINDOW;
 
  int x, y, width, height;
  if (::IsRectEmpty(&start_rect_)) {
    // Use the default window position/size.
    x = y = width = height = CW_USEDEFAULT;
  } else {
    x = start_rect_.left;
    y = start_rect_.top;
    width = start_rect_.right - start_rect_.left;
    height = start_rect_.bottom - start_rect_.top;
  }

  browser_settings_ = settings;

  // Create the main window initially hidden.
  CreateWindowEx(dwExStyle, window_class.c_str(), window_title.c_str(), dwStyle,
                 x, y, width, height, NULL, NULL, hInstance, this);
 
  if (!called_enable_non_client_dpi_scaling_ && IsProcessPerMonitorDpiAware()) {
    // This call gets Windows to scale the non-client area when WM_DPICHANGED
    // is fired on Windows versions < 10.0.14393.0.
    // Derived signature; not available in headers.
    typedef LRESULT(WINAPI * EnableChildWindowDpiMessagePtr)(HWND, BOOL);
    static EnableChildWindowDpiMessagePtr func_ptr =
        reinterpret_cast<EnableChildWindowDpiMessagePtr>(GetProcAddress(
            GetModuleHandle(L"user32.dll"), "EnableChildWindowDpiMessage"));
    if (func_ptr)
      func_ptr(hwnd_, TRUE);
  }

  if (!initially_hidden) {
    // Show this window.
    Show(no_activate ? ShowNoActivate : ShowNormal);
  }
}

// static
void RootWindowWin::RegisterRootClass(HINSTANCE hInstance,
                                      const std::wstring& window_class,
                                      HBRUSH background_brush) {
  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = RootWndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(131));
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = background_brush;
  wcex.lpszMenuName = MAKEINTRESOURCE(IDC_CEFCLIENT);
  wcex.lpszClassName = window_class.c_str();
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(131));

  RegisterClassEx(&wcex);
}

// static
LRESULT CALLBACK RootWindowWin::FindWndProc(HWND hWnd,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam) {
  REQUIRE_MAIN_THREAD();

  RootWindowWin* self = GetUserDataPtr<RootWindowWin*>(hWnd);
  DCHECK(self);
  DCHECK(hWnd == self->find_hwnd_);

  switch (message) {
    case WM_ACTIVATE:
      // Set this dialog as current when activated.
      MainMessageLoop::Get()->SetCurrentModelessDialog(wParam == 0 ? NULL
                                                                   : hWnd);
      return FALSE;
    case WM_NCDESTROY:
      // Clear the reference to |self|.
      SetUserDataPtr(hWnd, NULL);
      self->find_hwnd_ = NULL;
      break;
  }

  return CallWindowProc(self->find_wndproc_old_, hWnd, message, wParam, lParam);
}

// static
LRESULT CALLBACK RootWindowWin::RootWndProc(HWND hWnd,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam) {
  REQUIRE_MAIN_THREAD();

  RootWindowWin* self = NULL;
  if (message != WM_NCCREATE) {
    self = GetUserDataPtr<RootWindowWin*>(hWnd);
    if (!self)
      return DefWindowProc(hWnd, message, wParam, lParam);
    DCHECK_EQ(hWnd, self->hwnd_);
  }

  if (self && message == self->find_message_id_) {
    // Message targeting the find dialog.
    LPFINDREPLACE lpfr = reinterpret_cast<LPFINDREPLACE>(lParam);
    CHECK(lpfr == &self->find_state_);
    self->OnFindEvent();
    return 0;
  }

  if (self && self->with_osr_ && self->browser_window_)
  {
	  switch (message)
	  {
	  case WM_IME_SETCONTEXT:
	  case WM_IME_STARTCOMPOSITION:
	  case WM_IME_COMPOSITION:
	  case WM_IME_ENDCOMPOSITION:
	  case WM_LBUTTONDOWN:
	  case WM_RBUTTONDOWN:
	  case WM_MBUTTONDOWN:
	  case WM_LBUTTONUP:
	  case WM_RBUTTONUP:
	  case WM_MBUTTONUP:
	  case WM_MOUSEMOVE:
	  case WM_MOUSELEAVE:
	  case WM_MOUSEWHEEL:
	  case WM_SYSCHAR:
	  case WM_SYSKEYDOWN:
	  case WM_SYSKEYUP:
	  case WM_KEYDOWN:
	  case WM_KEYUP:
	  case WM_CHAR:
	  case WM_TOUCH:
	  case WM_PAINT:
      case WM_SIZE:
	  case WM_SETFOCUS:
	  case WM_KILLFOCUS:
		  return self->browser_window_->OsrWndProc(hWnd, message, wParam, lParam);
	  default:
		  break;
	  }
  }

  // Callback for the main window
  switch (message) {
  case WM_ACTIVATEAPP:
  {
	  //特殊处理, 因为用popup的子窗口模式 会有activity被 popup截走的bug
      if (hWnd == self->hwnd_) self->OnActiveApp(wParam);
  }
	  break;
    case WM_COMMAND:
      if (self->OnCommand(LOWORD(wParam)))
        return 0;
      break;

    case WM_GETOBJECT: {
      // Only the lower 32 bits of lParam are valid when checking the object id
      // because it sometimes gets sign-extended incorrectly (but not always).
      DWORD obj_id = static_cast<DWORD>(static_cast<DWORD_PTR>(lParam));

      // Accessibility readers will send an OBJID_CLIENT message.
      if (static_cast<DWORD>(OBJID_CLIENT) == obj_id) {
        if (self->GetBrowser() && self->GetBrowser()->GetHost())
          self->GetBrowser()->GetHost()->SetAccessibilityState(STATE_ENABLED);
      }
    } break;

    case WM_PAINT:
      self->OnPaint();
      return 0;

    case WM_ACTIVATE:
    {
        
        self->OnActivate(LOWORD(wParam) != WA_INACTIVE);
    }
      // Allow DefWindowProc to set keyboard focus.
      break;

    case WM_SETFOCUS:
      self->OnFocus();
      return 0;
	case WM_GETMINMAXINFO:
	{
		MONITORINFO oMonitor = {};
		oMonitor.cbSize = sizeof(oMonitor);
		::GetMonitorInfo(::MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &oMonitor);
		RECT rcWork = oMonitor.rcWork;

		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMaxPosition.x = 0;
		lpMMI->ptMaxPosition.y = 0;
		lpMMI->ptMaxSize.x = rcWork.right - rcWork.left;
		lpMMI->ptMaxSize.y = rcWork.bottom - rcWork.top;

		if (self->m_sMinSize.cx > 0) lpMMI->ptMinTrackSize.x = self->m_sMinSize.cx;
		if (self->m_sMinSize.cy > 0) lpMMI->ptMinTrackSize.y = self->m_sMinSize.cy;
	}
	break;
    case WM_SIZE:
        self->OnSize(wParam == SIZE_MINIMIZED);
        break;
	case WM_SYSCOMMAND:
	{
		if (wParam == SC_MINIMIZE)
			self->OnMinimize();
	}
		break;
    case WM_MOVING:
    case WM_MOVE:
      self->OnMove();
      return 0;

    case WM_DPICHANGED:
      self->OnDpiChanged(wParam, lParam);
      break;

    case WM_ERASEBKGND:
      if (self->OnEraseBkgnd())
        break;
      // Don't erase the background.
      return 0;

    case WM_ENTERMENULOOP:
      if (!wParam) {
        // Entering the menu loop for the application menu.
        CefSetOSModalLoop(true);
      }
      break;

    case WM_EXITMENULOOP:
      if (!wParam) {
        // Exiting the menu loop for the application menu.
        CefSetOSModalLoop(false);
      }
      break;

    case WM_CLOSE:
        if (self->OnClose((int)wParam))
            return 0;  // Cancel the close.
      break;

    case WM_NCHITTEST: {
      LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
      if (hit == HTCLIENT) {
        POINTS points = MAKEPOINTS(lParam);
        POINT point = {points.x, points.y};
        ::ScreenToClient(hWnd, &point);

        RECT rcClt;
        GetClientRect(hWnd, &rcClt);

        int nRegionWidth = 5;

        if(self->m_bResizable)
		{
			if (point.x < nRegionWidth) {
				if (point.y < nRegionWidth) return HTTOPLEFT;
				else if (point.y > rcClt.bottom - nRegionWidth) return HTBOTTOMLEFT;
				else return HTLEFT;
			}
			else if (point.x > rcClt.right - nRegionWidth) {
				if (point.y < nRegionWidth) return HTTOPRIGHT;
				else if (point.y > rcClt.bottom - nRegionWidth) return HTBOTTOMRIGHT;
				else return HTRIGHT;
			}
			else if (point.y < nRegionWidth) return HTTOP;
			else if (point.y > rcClt.bottom - nRegionWidth) return HTBOTTOM;
		}

        if (::PtInRegion(self->draggable_region_, point.x, point.y)) {
          // If cursor is inside a draggable region return HTCAPTION to allow
          // dragging.
          return HTCAPTION;
        }
      }
      return hit;
    }

    case WM_NCCREATE: {
      CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
      self = reinterpret_cast<RootWindowWin*>(cs->lpCreateParams);
      DCHECK(self);
      // Associate |self| with the main window.
      SetUserDataPtr(hWnd, self);
      self->hwnd_ = hWnd;

      self->OnNCCreate(cs);
    } break;

    case WM_CREATE:
      self->OnCreate(reinterpret_cast<CREATESTRUCT*>(lParam));
      break;

    case WM_NCDESTROY:
      // Clear the reference to |self|.
      SetUserDataPtr(hWnd, NULL);
      self->hwnd_ = NULL;
      self->OnDestroyed();
      break;
    case WM_NCCALCSIZE:
        if(self->isDevsTool) break;
        else return 0;
    case WM_USER + 1002:
        self->OnLostFoucs();
        break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

void RootWindowWin::OnPaint() {
  PAINTSTRUCT ps;
  BeginPaint(hwnd_, &ps);
  EndPaint(hwnd_, &ps);
}

void RootWindowWin::OnFocus() {
  // Selecting "Close window" from the task bar menu may send a focus
  // notification even though the window is currently disabled (e.g. while a
  // modal JS dialog is displayed).
  if (browser_window_ && ::IsWindowEnabled(hwnd_))
    browser_window_->SetFocus(true);
}

void RootWindowWin::OnActivate(bool active) {
  if (active)
    delegate_->OnRootWindowActivated(this);
}

void RootWindowWin::OnSize(bool minimized) {
  if (minimized) {
    // Notify the browser window that it was hidden and do nothing further.
    if (browser_window_)
      browser_window_->Hide();
    return;
  }

  if (browser_window_)
    browser_window_->Show();

  RECT rect;
  GetClientRect(hwnd_, &rect);
 
  if (browser_window_) {
         browser_window_->SetBounds(0, 0, rect.right, rect.bottom);
  }

  std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
  for (; it != vChildBrowsers.end(); it++) {
	  BrowserWindow* pBrowserWindow = *it;

	  RECT rcMargin = pBrowserWindow->GetMarginRect();

	  RECT rcParent = { 0 };
	  GetWindowRect(hwnd_, &rcParent);

      float fScal = GetWindowScaleFactor(hwnd_);

	  int nWidth = rcParent.right - rcParent.left - (rcMargin.left + rcMargin.right) * fScal;
	  int nheight = rcParent.bottom - rcParent.top - (rcMargin.top + rcMargin.bottom) * fScal;

	  pBrowserWindow->SetBounds(rcMargin.left*fScal , rcMargin.top * fScal, nWidth, nheight);
	  
	  if (pBrowserWindow->IsSelected())
		  pBrowserWindow->TabShow();
  }

  if (m_ToastWnd.IsVisible()) m_ToastWnd.UpdatePos();
}

void RootWindowWin::OnMinimize()
{
	std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	for (; it != vChildBrowsers.end(); it++) {
		BrowserWindow* pBrowserWindow = *it;
		if (pBrowserWindow->IsSelected())
		{
			pBrowserWindow->TabHide();
			break;
		}
	}
}

void RootWindowWin::OnMove() {
  // Notify the browser of move events so that popup windows are displayed
  // in the correct location and dismissed when the window moves.
  CefRefPtr<CefBrowser> browser = GetBrowser();
  if (browser) {
      browser->GetHost()->NotifyMoveOrResizeStarted();
  }

  std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
  for (; it != vChildBrowsers.end(); it++) {
	  BrowserWindow* pBrowserWindow = *it;
	  RECT rcMargin = pBrowserWindow->GetMarginRect();
	  float fScal = GetWindowScaleFactor(hwnd_);
	  pBrowserWindow->SetBounds(rcMargin.left * fScal, rcMargin.top * fScal, 0, 0);
  }
  if (m_ToastWnd.IsVisible()) m_ToastWnd.UpdatePos();
}

void RootWindowWin::OnDpiChanged(WPARAM wParam, LPARAM lParam) {
  if (LOWORD(wParam) != HIWORD(wParam)) {
    NOTIMPLEMENTED() << "Received non-square scaling factors";
    return;
  }

  if (browser_window_ && with_osr_) {
    // Scale factor for the new display.
    const float display_scale_factor =
        static_cast<float>(LOWORD(wParam)) / DPI_1X;
    browser_window_->SetDeviceScaleFactor(display_scale_factor);
  }

  // Suggested size and position of the current window scaled for the new DPI.
  const RECT* rect = reinterpret_cast<RECT*>(lParam);
  SetBounds(rect->left, rect->top, rect->right - rect->left,
            rect->bottom - rect->top);
}

bool RootWindowWin::OnEraseBkgnd() {
  // Erase the background when the browser does not exist.
  return (GetBrowser() == NULL);
}

bool RootWindowWin::OnCommand(UINT id) {
  if (id >= ID_TESTS_FIRST && id <= ID_TESTS_LAST) {
    delegate_->OnTest(this, id);
    return true;
  }

  switch (id) {
    case IDM_EXIT:
      delegate_->OnExit(this);
      return true;
    case ID_FIND:
      OnFind();
      return true;
    case IDC_NAV_BACK:  // Back button
      if (CefRefPtr<CefBrowser> browser = GetBrowser())
        browser->GoBack();
      return true;
    case IDC_NAV_FORWARD:  // Forward button
      if (CefRefPtr<CefBrowser> browser = GetBrowser())
        browser->GoForward();
      return true;
    case IDC_NAV_RELOAD:  // Reload button
      if (CefRefPtr<CefBrowser> browser = GetBrowser())
        browser->Reload();
      return true;
    case IDC_NAV_STOP:  // Stop button
      if (CefRefPtr<CefBrowser> browser = GetBrowser())
        browser->StopLoad();
      return true;
  }

  return false;
}

void RootWindowWin::OnFind() {
  if (find_hwnd_) {
    // Give focus to the existing find dialog.
    ::SetFocus(find_hwnd_);
    return;
  }

  // Configure dialog state.
  ZeroMemory(&find_state_, sizeof(find_state_));
  find_state_.lStructSize = sizeof(find_state_);
  find_state_.hwndOwner = hwnd_;
  find_state_.lpstrFindWhat = find_buff_;
  find_state_.wFindWhatLen = sizeof(find_buff_);
  find_state_.Flags = FR_HIDEWHOLEWORD | FR_DOWN;

  // Create the dialog.
  find_hwnd_ = FindText(&find_state_);

  // Override the dialog's window procedure.
  find_wndproc_old_ = SetWndProcPtr(find_hwnd_, FindWndProc);

  // Associate |self| with the dialog.
  SetUserDataPtr(find_hwnd_, this);
}

void RootWindowWin::OnFindEvent() {
  CefRefPtr<CefBrowser> browser = GetBrowser();

  if (find_state_.Flags & FR_DIALOGTERM) {
    // The find dialog box has been dismissed so invalidate the handle and
    // reset the search results.
    if (browser) {
      browser->GetHost()->StopFinding(true);
      find_what_last_.clear();
      find_next_ = false;
    }
  } else if ((find_state_.Flags & FR_FINDNEXT) && browser) {
    // Search for the requested string.
    bool match_case = ((find_state_.Flags & FR_MATCHCASE) ? true : false);
    const std::wstring& find_what = find_buff_;
    if (match_case != find_match_case_last_ || find_what != find_what_last_) {
      // The search string has changed, so reset the search results.
      if (!find_what.empty()) {
        browser->GetHost()->StopFinding(true);
        find_next_ = false;
      }
      find_match_case_last_ = match_case;
      find_what_last_ = find_buff_;
    }

    browser->GetHost()->Find(0, find_what,
                             (find_state_.Flags & FR_DOWN) ? true : false,
                             match_case, find_next_);
    if (!find_next_)
      find_next_ = true;
  }
}


void RootWindowWin::OnNCCreate(LPCREATESTRUCT lpCreateStruct) {
  if (IsProcessPerMonitorDpiAware()) {
    // This call gets Windows to scale the non-client area when WM_DPICHANGED
    // is fired on Windows versions >= 10.0.14393.0.
    typedef BOOL(WINAPI * EnableNonClientDpiScalingPtr)(HWND);
    static EnableNonClientDpiScalingPtr func_ptr =
        reinterpret_cast<EnableNonClientDpiScalingPtr>(GetProcAddress(
            GetModuleHandle(L"user32.dll"), "EnableNonClientDpiScaling"));
    called_enable_non_client_dpi_scaling_ = !!(func_ptr && func_ptr(hwnd_));
  }
}

void EnableFlash(CefRefPtr<CefRequestContext> reqCtx)
{
	CefString error;
	CefRefPtr<CefValue> value = CefValue::Create();
	value->SetInt(1);
	reqCtx->SetPreference("profile.default_content_setting_values.plugins", value, error);
}

void RootWindowWin::OnCreate(LPCREATESTRUCT lpCreateStruct) {
 
// No controls so also remove the default menu.
::SetMenu(hwnd_, NULL);


  const float device_scale_factor = GetWindowScaleFactor(hwnd_);

  if (with_osr_) {
    browser_window_->SetDeviceScaleFactor(device_scale_factor);
  }

  RECT rect;
  GetClientRect(hwnd_, &rect);

  if (!is_popup_) {
    // Create the browser window.
    CefRect cef_rect(rect.left, rect.top, rect.right - rect.left,
                     rect.bottom - rect.top);
    auto reqCtx = delegate_->GetRequestContext(this,false);
    CefPostTask(TID_UI, base::Bind(EnableFlash, reqCtx));
    browser_window_->CreateBrowser(hwnd_, cef_rect, browser_settings_, NULL,reqCtx);
  } else {
    // With popups we already have a browser window. Parent the browser window
    // to the root window and show it in the correct location.
    browser_window_->ShowPopup(hwnd_, rect.left, rect.top,
                               rect.right - rect.left, rect.bottom - rect.top);
    is_popup_ = false;
  }
}



bool RootWindowWin::OnClose(int nID) {

	if (browser_window_ && !browser_window_->IsClosing()) {

        if (vChildBrowsers.size())
		{
			for (UINT i = 0; i < vChildBrowsers.size(); i++)
			{
				if (!vChildBrowsers[i]->IsClosing()) {
                    CloseTabBrowser(vChildBrowsers[i]->GetBrowserID());
				}
			}
		}
        
		CefRefPtr<CefBrowser> browser = GetBrowser();
		if (browser) {
			// Notify the browser window that we would like to close it. This
			// will result in a call to ClientHandler::DoClose() if the
			// JavaScript 'onbeforeunload' event handler allows it.
			browser->GetHost()->CloseBrowser(false);

			// Cancel the close.
			return true;
		}
    }
    else if (vChildBrowsers.size()) return true;
	// Allow the close.
	return false;

}

void RootWindowWin::OnDestroyed() {
  window_destroyed_ = true;
  NotifyDestroyedIfDone();
}

void RootWindowWin::OnLostFoucs()
{
    switch (m_nKillFocusAction)
    {
    case 0:
        break;
    case 1:
        Hide();
        break;
    case 2:
        Close(false);
        break;
    default:
        break;
    }
}

void RootWindowWin::OnActiveApp(bool bActive)
{
    //基于自身的多标签窗口 用ws_child 会有绘制问题, 所以用popup配合ws_child联动, 但是这样又会有焦点问题
    //具体表现在  焦点在 popup子窗口中 app deactive后 在任务栏恢复  没法恢复
    //因为 activeapp后 还有个 active被子窗口吃掉了
    //又不能让子窗口发出来, 那样会导致 网页内有edit时有bug
    //但是直接 在activeapp改 foreground也不行, 因为 每次激活 程序的所有顶层窗口 都会收到 activeapp

    //所以这里有一个 机制    第一个activeapp 享受 额外foreground待遇,  它deactive时候记为一个周期
    if (!hCurForgroundWnd || hCurForgroundWnd == hwnd_) 
        MainContext::Get()->GetRootWindowManager()->OnActiveApp(bActive, hwnd_);
}

void RootWindowWin::OnAppActived(bool bActive, HWND hWnd)
{
    if (bActive)
    {
        if (hWnd == hwnd_) SetForegroundWindow(hwnd_);
        hCurForgroundWnd = hWnd;
    }
    else
    {
        hCurForgroundWnd = NULL;
    }
}

LRESULT CALLBACK FocusMonitProc(HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	WNDPROC hParentWndProc =
		reinterpret_cast<WNDPROC>(::GetPropW(hWnd, L"OLDPROC"));
	switch (message)
	{
	case WM_KILLFOCUS:
    {
        HWND hParent = reinterpret_cast<HWND>(::GetPropW(hWnd, L"OWNERWND"));
        PostMessage(hParent, WM_USER + 1002, 0, 0);
    }
		break;
	default:
		break;
	}
	if (hParentWndProc) return CallWindowProc(hParentWndProc, hWnd, message, wParam, lParam);
	else return DefWindowProc(hWnd, message, wParam, lParam);
}

void RootWindowWin::OnBrowserCreated(CefRefPtr<CefBrowser> browser, BrowserWindow *pBrowserWindow) {
  REQUIRE_MAIN_THREAD();

  CefRefPtr<CefMessageRouterBrowserSide::Callback> fCB = browser_window_->GetObserFunc();
  if (fCB) {
      CefRefPtr<CefDictionaryValue> resp = CefDictionaryValue::Create();
	  resp->SetString("name", "onTabBrowserCreated");

      CefRefPtr<CefDictionaryValue> arg = CefDictionaryValue::Create();
	  if (browser)
	  {
		  CefRefPtr<CefClient> cefClient = browser->GetHost()->GetClient();
		  if (cefClient) {
			  ClientHandler* pHandler = (ClientHandler*)(cefClient.get());
			  arg->SetString("tag", pHandler->GetUniqeTag());
			  arg->SetString("group", pHandler->GetGroupTag());
		  }
		  arg->SetInt("browserid", browser->GetIdentifier());
          arg->SetBool("ie", false);
	  }
	  else if (pBrowserWindow)
	  {
		  arg->SetInt("browserid", pBrowserWindow->GetBrowserID());
		  arg->SetString("tag", pBrowserWindow->GetUniqeTag());
		  arg->SetString("group", pBrowserWindow->GetBrowserGroup());
		  arg->SetBool("ie", true);
	  }

      resp->SetDictionary("arg", arg);
	  
	  CefRefPtr<CefValue> value = CefValue::Create();
	  value->SetDictionary(resp);
      CefString szResp = CefWriteJSON(value, JSON_WRITER_DEFAULT);
      fCB->Success(szResp);
  }

  //主browser创建成功;
  if(browser && browser->GetIdentifier() == browser_window_->GetBrowserID())
  {
      if(isAsTool)
	  {
          HWND hTMoniteTarget = NULL;
		  if (!with_osr_)
		  {
              hTMoniteTarget = GetWindow(hwnd_, GW_CHILD);
              if(hTMoniteTarget) hTMoniteTarget = GetWindow(hTMoniteTarget, GW_CHILD);
		  }
          else
          {
              hTMoniteTarget = hwnd_;
          }
     
          if(hTMoniteTarget)
		  {
			  LONG_PTR hOldWndProc = SetWindowLongPtr(hTMoniteTarget, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(FocusMonitProc));
			  if (hOldWndProc == 0 && GetLastError() != ERROR_SUCCESS) {
				  return;
			  }
			  ::SetPropW(hTMoniteTarget, L"OLDPROC", reinterpret_cast<HANDLE>(hOldWndProc));
			  ::SetPropW(hTMoniteTarget, L"OWNERWND", reinterpret_cast<HANDLE>(hwnd_));
		  }
         
	  }


	  if (browser_window_->GetUniqeTag().empty()) {
          if (isDevsTool) browser_window_->SetUniqeTag("devstool");
          else browser_window_->SetUniqeTag(MainContextImpl::Get()->GetInitFrameName());
	  }
      nMainBrowserID = browser->GetIdentifier();
      szMainBrowserTag = browser_window_->GetUniqeTag();
	  if (is_popup_) {
		  // For popup browsers create the root window once the browser has been
		  // created.
		  CreateRootWindow(CefBrowserSettings(), false);
	  }
	  else {
		  // Make sure the browser is sized correctly.
		  OnSize(false);
          Show(ShowNormal);
	  }
	  delegate_->OnBrowserCreated(this, browser);
  }
}

void RootWindowWin::OnBrowserWindowDestroyed(int nBrowserID) {
  REQUIRE_MAIN_THREAD();

  if(nBrowserID == nMainBrowserID)
  {
	  browser_window_.reset();
	  if (!window_destroyed_) {
		  // The browser was destroyed first. This could be due to the use of
		  // off-screen rendering or execution of JavaScript window.close().
		  // Close the RootWindow.
		  Close(true);
	  }
	  browser_destroyed_ = true;
	  NotifyDestroyedIfDone();
  }
  else {

	  std::vector<BrowserWindow*>::iterator it = vChildBrowsers.begin();
	  for (; it != vChildBrowsers.end(); it++) {
		  BrowserWindow* pBrowserWindow = *it;
          if (pBrowserWindow->GetBrowserID() == nBrowserID)
          {
              CefRefPtr<CefMessageRouterBrowserSide::Callback> fCB = browser_window_->GetObserFunc();
			  if (fCB) {
				  CefRefPtr<CefDictionaryValue> resp = CefDictionaryValue::Create();
				  resp->SetString("name", "onTabBrowserClosed");
				  CefRefPtr<CefDictionaryValue> arg = CefDictionaryValue::Create();
				  arg->SetInt("browserid", nBrowserID);
                  arg->SetString("tag", pBrowserWindow->GetUniqeTag());
				  resp->SetDictionary("arg", arg);
				  CefRefPtr<CefValue> value = CefValue::Create();
				  value->SetDictionary(resp);
				  CefString szResp = CefWriteJSON(value, JSON_WRITER_DEFAULT);
                  fCB->Success(szResp);
			  }

			  pBrowserWindow->ReleaseHostWndIfTab();
              vChildBrowsers.erase(it);
			  delete pBrowserWindow;
			  pBrowserWindow = NULL;
              break;
          }
	  }
      if (!vChildBrowsers.size() && browser_window_->IsClosing()) {
          PostMessage(browser_window_->GetWindowHandle(), WM_CLOSE, nBrowserID, 0);
      }
  }
}

void RootWindowWin::OnBrowserNavigate(CefRefPtr<CefBrowser> browser, std::string sTargetTag, std::string szTargetURL)
{
    if (m_FullTopManager.IsFullScreen()) m_FullTopManager.RestoreWnd();

	CefRefPtr<CefDictionaryValue> resp = CefDictionaryValue::Create();
	resp->SetString("name", "onOpenURL");

    int nBID = browser->GetIdentifier();
	CefRefPtr<CefDictionaryValue> arg = CefDictionaryValue::Create();

	arg->SetInt("senderid", nBID);
    arg->SetString("sendertag", GetBrowserTagFromID(nBID));
    arg->SetString("targeturl", szTargetURL);
	resp->SetDictionary("arg", arg);

	CefRefPtr<CefValue> value = CefValue::Create();
	value->SetDictionary(resp);
	CefString szResp = CefWriteJSON(value, JSON_WRITER_DEFAULT);
    CefRefPtr<CefMessageRouterBrowserSide::Callback> fCB = browser_window_->GetObserFunc();
    if(fCB) fCB->Success(szResp);
}

void RootWindowWin::OnSetAddress(const std::string& url) {
  REQUIRE_MAIN_THREAD();

  if (edit_hwnd_)
    SetWindowText(edit_hwnd_, CefString(url).ToWString().c_str());
}

void RootWindowWin::OnSetTitle(const std::string& title) {
  REQUIRE_MAIN_THREAD();

  if (hwnd_)
    SetWindowText(hwnd_, CefString(title).ToWString().c_str());
}

void RootWindowWin::OnSetFullscreen(bool fullscreen) {
  REQUIRE_MAIN_THREAD();
  if (hwnd_)
  {
	  if (IsZoomed(hwnd_)) PostMessage(hwnd_, WM_SYSCOMMAND, SC_RESTORE, 0);
	  else PostMessage(hwnd_, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
  }
}

void RootWindowWin::OnAutoResize(const CefSize& new_size) {
  REQUIRE_MAIN_THREAD();

  if (!hwnd_)
    return;

  int new_width = new_size.width;

  // Make the window wide enough to drag by the top menu bar.
  if (new_width < 200)
    new_width = 200;

  const float device_scale_factor = GetWindowScaleFactor(hwnd_);
  RECT rect = {0, 0, LogicalToDevice(new_width, device_scale_factor),
               LogicalToDevice(new_size.height, device_scale_factor)};
  DWORD style = GetWindowLong(hwnd_, GWL_STYLE);
  DWORD ex_style = GetWindowLong(hwnd_, GWL_EXSTYLE);
  bool has_menu = !(style & WS_CHILD) && (GetMenu(hwnd_) != NULL);

  // The size value is for the client area. Calculate the whole window size
  // based on the current style.
  AdjustWindowRectEx(&rect, style, has_menu, ex_style);

  // Size the window. The left/top values may be negative.
  // Also show the window if it's not currently visible.
  SetWindowPos(hwnd_, NULL, 0, 0, rect.right - rect.left,
               rect.bottom - rect.top,
               SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void RootWindowWin::OnSetLoadingState(bool isLoading,
                                      bool canGoBack,
                                      bool canGoForward) {
  REQUIRE_MAIN_THREAD();

  if (with_controls_) {
    EnableWindow(back_hwnd_, canGoBack);
    EnableWindow(forward_hwnd_, canGoForward);
    EnableWindow(reload_hwnd_, !isLoading);
    EnableWindow(stop_hwnd_, isLoading);
    EnableWindow(edit_hwnd_, TRUE);
  }

  if (!isLoading && GetWindowLongPtr(hwnd_, GWL_EXSTYLE) & WS_EX_NOACTIVATE) {
    // Done with the initial navigation. Remove the WS_EX_NOACTIVATE style so
    // that future mouse clicks inside the browser correctly activate and focus
    // the window. For the top-level window removing this style causes Windows
    // to display the task bar button.
    SetWindowLongPtr(hwnd_, GWL_EXSTYLE,
                     GetWindowLongPtr(hwnd_, GWL_EXSTYLE) & ~WS_EX_NOACTIVATE);

    if (browser_window_) {
      HWND browser_hwnd = browser_window_->GetWindowHandle();
      SetWindowLongPtr(
          browser_hwnd, GWL_EXSTYLE,
          GetWindowLongPtr(browser_hwnd, GWL_EXSTYLE) & ~WS_EX_NOACTIVATE);
    }
  }
}

namespace {

LPCWSTR kParentWndProc = L"CefParentWndProc";
LPCWSTR kDraggableRegion = L"CefDraggableRegion";

LRESULT CALLBACK SubclassedWindowProc(HWND hWnd,
                                      UINT message,
                                      WPARAM wParam,
                                      LPARAM lParam) {
  WNDPROC hParentWndProc =
      reinterpret_cast<WNDPROC>(::GetPropW(hWnd, kParentWndProc));
  HRGN hRegion = reinterpret_cast<HRGN>(::GetPropW(hWnd, kDraggableRegion));

  if (message == WM_NCHITTEST) {

    
    LRESULT hit = CallWindowProc(hParentWndProc, hWnd, message, wParam, lParam);
    if (hit == HTCLIENT) {

        RECT rcWnd;
        GetClientRect(hWnd, &rcWnd);

      POINTS points = MAKEPOINTS(lParam);
      POINT point = {points.x, points.y};
      ::ScreenToClient(hWnd, &point);

      int nTDragWidth = 5;
      if (::PtInRegion(hRegion, point.x, point.y) || point.x < nTDragWidth || point.x > rcWnd.right - nTDragWidth || point.y < nTDragWidth || point.y > rcWnd.bottom - nTDragWidth) {
        // Let the parent window handle WM_NCHITTEST by returning HTTRANSPARENT
        // in child windows.
        return HTTRANSPARENT;
      }
    }
    return hit;
  }

  return CallWindowProc(hParentWndProc, hWnd, message, wParam, lParam);
}

void SubclassWindow(HWND hWnd, HRGN hRegion) {
  HANDLE hParentWndProc = ::GetPropW(hWnd, kParentWndProc);
  if (hParentWndProc) {
    return;
  }

  SetLastError(0);
  LONG_PTR hOldWndProc = SetWindowLongPtr(
      hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SubclassedWindowProc));
  if (hOldWndProc == 0 && GetLastError() != ERROR_SUCCESS) {
    return;
  }

  ::SetPropW(hWnd, kParentWndProc, reinterpret_cast<HANDLE>(hOldWndProc));
  ::SetPropW(hWnd, kDraggableRegion, reinterpret_cast<HANDLE>(hRegion));
}

void UnSubclassWindow(HWND hWnd) {
  LONG_PTR hParentWndProc =
      reinterpret_cast<LONG_PTR>(::GetPropW(hWnd, kParentWndProc));
  if (hParentWndProc) {
    LONG_PTR hPreviousWndProc =
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, hParentWndProc);
    ALLOW_UNUSED_LOCAL(hPreviousWndProc);
    DCHECK_EQ(hPreviousWndProc,
              reinterpret_cast<LONG_PTR>(SubclassedWindowProc));
  }

  ::RemovePropW(hWnd, kParentWndProc);
  ::RemovePropW(hWnd, kDraggableRegion);
}

BOOL CALLBACK SubclassWindowsProc(HWND hwnd, LPARAM lParam) {
    wchar_t curClsName[MAX_PATH] = { 0 };
    wchar_t parenClsName[MAX_PATH] = { 0 };

    GetClassName(hwnd, curClsName, MAX_PATH);
    GetClassName(GetParent(hwnd), parenClsName, MAX_PATH);
    if (!wcscmp(curClsName, L"CefBrowserWindow"))
    {
        if (!wcscmp(parenClsName, L"Chrome_RenderWidgetHostHWND"))
        {
            SetPropW(hwnd, L"CHILD_HIT_PASS", (HANDLE)1);
            return TRUE;
        }
    }
    HANDLE hRes = GetPropW(GetParent(hwnd), L"CHILD_HIT_PASS");
    if (hRes)
        return TRUE;
  SubclassWindow(hwnd, reinterpret_cast<HRGN>(lParam));
  return TRUE;
}

BOOL CALLBACK UnSubclassWindowsProc(HWND hwnd, LPARAM lParam) {
  UnSubclassWindow(hwnd);
  return TRUE;
}

}  // namespace

void RootWindowWin::OnSetDraggableRegions(
    const std::vector<CefDraggableRegion>& regions) {
  REQUIRE_MAIN_THREAD();
  
  // Reset draggable region.
  ::SetRectRgn(draggable_region_, 0, 0, 0, 0);

  float fScal = GetWindowScaleFactor(hwnd_);
  // Determine new draggable region.
  std::vector<CefDraggableRegion>::const_iterator it = regions.begin();
  for (; it != regions.end(); ++it) 
  {
      int nX = it->bounds.x * fScal;
      int nY = it->bounds.y * fScal;
      int nWidth = it->bounds.width * fScal;
      int nHeight = it->bounds.height * fScal;

    HRGN region = ::CreateRectRgn(nX, nY,nX + nWidth,nY + nHeight);
    ::CombineRgn(draggable_region_, draggable_region_, region,it->draggable ? RGN_OR : RGN_DIFF);
    ::DeleteObject(region);
  }

  // Subclass child window procedures in order to do hit-testing.
  // This will be a no-op, if it is already subclassed.
  if (hwnd_) {
    WNDENUMPROC proc =
        !regions.empty() ? SubclassWindowsProc : UnSubclassWindowsProc;
    ::EnumChildWindows(hwnd_, proc,
                       reinterpret_cast<LPARAM>(draggable_region_));
  }
}

void RootWindowWin::NotifyDestroyedIfDone() {
  // Notify once both the window and the browser have been destroyed.
  if (window_destroyed_ && browser_destroyed_)
    delegate_->OnRootWindowDestroyed(this);
}

}  // namespace client
