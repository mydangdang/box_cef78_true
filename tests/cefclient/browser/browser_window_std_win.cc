// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "tests/cefclient/browser/browser_window_std_win.h"

#include "tests/cefclient/browser/client_handler_std.h"
#include "tests/shared/browser/main_message_loop.h"
#include "include/cef_task.h"
#include "../string_util.h"


namespace client {

	const wchar_t* g_ChromeTabHostWndCls = L"ChromeKernelBrowserHost";

	LRESULT WINAPI ChromeHostWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		BrowserWindowStdWin* pBrowserWindow = (BrowserWindowStdWin*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		switch (uMsg)
		{
		case WM_NCCREATE:
		{
			CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
			BrowserWindowStdWin* pThis = reinterpret_cast<BrowserWindowStdWin*>(cs->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		}
		break;
		case WM_NCDESTROY:
		{
			SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
		}
		break;
		case WM_SIZE:
			if(pBrowserWindow) pBrowserWindow->OnTabHostSizeChange(wParam == SIZE_MINIMIZED);
			break;
		case WM_DPICHANGED:
		{
			if (LOWORD(wParam) != HIWORD(wParam)) break;
			if (pBrowserWindow)
			{
				const RECT* rect = reinterpret_cast<RECT*>(lParam);
				SetWindowPos(hWnd, NULL, rect->left, rect->top, static_cast<int>(rect->right - rect->left),static_cast<int>(rect->bottom - rect->top), SWP_NOZORDER);
			}
		}
			break;
		case WM_ERASEBKGND:
			return 0;
		default:
			break;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}


BrowserWindowStdWin::BrowserWindowStdWin(Delegate* delegate,
                                         const std::string& startup_url,
                                         const std::string& tag,
                                         const std::string& group,
                                         bool bIE,
										 int nFixFlash,
										 bool bTab)
    : BrowserWindow(delegate) {
    __super::SetUniqeTag(tag);
    __super::SetBrowserGroup(group);
    m_bIEKernel = bIE;
	m_bTopFull = false;
	m_bTabWindow = bTab;
	if (m_bTabWindow)
	{
		WNDCLASS wc;
		wc.cbClsExtra = wc.cbWndExtra = 0;
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hIcon = NULL;
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpfnWndProc = ChromeHostWndProc;
		wc.lpszClassName = g_ChromeTabHostWndCls;
		wc.lpszMenuName = NULL;
		wc.style = 0;
		RegisterClass(&wc);
	}
	m_hTabHostWnd = NULL;
	ie_client_handler = NULL;
    if(!m_bIEKernel)
	{
		client_handler_ = new ClientHandlerStd(this, startup_url);
		client_handler_->SetUniqeTag(tag);
		client_handler_->SetGroupTag(group);
    }
	else
	{
		ie_client_handler = new IEBrowserHandler(this, startup_url);
		ie_client_handler->SetFixFlash(nFixFlash);
	}
}

void BrowserWindowStdWin::CreateBrowser(
    ClientWindowHandle parent_handle,
    const CefRect& rect,
    const CefBrowserSettings& settings,
    CefRefPtr<CefDictionaryValue> extra_info,
    CefRefPtr<CefRequestContext> request_context) {
  REQUIRE_MAIN_THREAD();

  if(!m_bIEKernel)
  {
	  
	  CefWindowInfo window_info;
	  if (!m_bTabWindow)
	  {
		  RECT wnd_rect = { rect.x, rect.y, rect.x + rect.width, rect.y + rect.height };
		  window_info.SetAsChild(parent_handle, wnd_rect);

		  if (GetWindowLongPtr(parent_handle, GWL_EXSTYLE) & WS_EX_NOACTIVATE) {
			  // Don't activate the browser window on creation.
			  window_info.ex_style |= WS_EX_NOACTIVATE;
		  }
	  }
	  else
	  {
		  HWND hAncestor = GetAncestor(parent_handle, GA_ROOT);
		  RECT rcAncestor = { 0 };
		  GetWindowRect(hAncestor, &rcAncestor);
		  int nX = rect.x + rcAncestor.left;
		  int nY = rect.y + rcAncestor.top;

		  m_hTabHostWnd = CreateWindowEx(WS_EX_TOOLWINDOW, g_ChromeTabHostWndCls, g_ChromeTabHostWndCls,
			  WS_CHILDWINDOW | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE,
			  nX, nY, rect.width, rect.height, parent_handle, NULL, GetModuleHandle(NULL), this);

		  RECT wnd_rect = { 0,0, rect.width, rect.height };
		  window_info.SetAsChild(m_hTabHostWnd, wnd_rect);

		  if (GetWindowLongPtr(m_hTabHostWnd, GWL_EXSTYLE) & WS_EX_NOACTIVATE) {
			  // Don't activate the browser window on creation.
			  window_info.ex_style |= WS_EX_NOACTIVATE;
		  }
	  }
	  CefBrowserHost::CreateBrowser(window_info, client_handler_,
		  client_handler_->startup_url(), settings,
		  extra_info, request_context);
  }
  else {
      ie_client_handler->CreateBrowser(parent_handle, rect);
      //todo  µ÷ÓÃonbrowsercreate
      nBrowserID = ie_client_handler->GetBrowserID();
	  if(delegate_) delegate_->OnBrowserCreated(NULL, this);
  }
}

void BrowserWindowStdWin::GetPopupConfig(CefWindowHandle temp_handle,
                                         CefWindowInfo& windowInfo,
                                         CefRefPtr<CefClient>& client,
                                         CefBrowserSettings& settings) {
  CEF_REQUIRE_UI_THREAD();

  // The window will be properly sized after the browser is created.
  windowInfo.SetAsChild(temp_handle, RECT());

  // Don't activate the hidden browser window on creation.
  windowInfo.ex_style |= WS_EX_NOACTIVATE;

  client = client_handler_;
}

void BrowserWindowStdWin::ShowPopup(ClientWindowHandle parent_handle,
                                    int x,
                                    int y,
                                    size_t width,
                                    size_t height) {
  REQUIRE_MAIN_THREAD();

  HWND hwnd = GetWindowHandle();
  if (hwnd) {
    SetParent(hwnd, parent_handle);
    SetWindowPos(hwnd, NULL, x, y, static_cast<int>(width),
                 static_cast<int>(height), SWP_NOZORDER | SWP_NOACTIVATE);

    const bool no_activate = GetWindowLongPtr(parent_handle, GWL_EXSTYLE) & WS_EX_NOACTIVATE;
    ShowWindow(hwnd, no_activate ? SW_SHOWNOACTIVATE : SW_SHOW);
  }
}


void BrowserWindowStdWin::TabShow()
{
	HWND hwnd = NULL;
	if (!m_bIEKernel) hwnd = GetWindowHandle();
	else
	{   
		if(ie_client_handler) hwnd = ie_client_handler->GetHostWnd();
	}
	if (hwnd && !::IsWindowVisible(hwnd))
	{
		bool bIE = IsIEKernel();
		if(!bIE && !m_bTabWindow) 
			SetWindowPos(hwnd, NULL, m_rcOld.left, m_rcOld.top, m_rcOld.right - m_rcOld.left, m_rcOld.bottom - m_rcOld.top,SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
		ShowWindow(hwnd, SW_SHOW);

#ifdef _DEBUG
		char szOut[1024] = { 0 };
		sprintf_s(szOut, "ShowBrowser %x\n", hwnd);
		OutputDebugStringA(szOut);
#endif
	}
}

void BrowserWindowStdWin::TabHide()
{
	HWND hwnd = NULL;
	if(!m_bIEKernel)
		hwnd = GetWindowHandle();
	else
	{
		if(ie_client_handler) hwnd = ie_client_handler->GetHostWnd();
	}
	if (hwnd && ::IsWindowVisible(hwnd))
	{
		if (!IsIEKernel() && !m_bTabWindow)
		{
			GetWindowRect(hwnd, &m_rcOld);
			SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
		}
		ShowWindow(hwnd, SW_HIDE);
#ifdef _DEBUG
		char szOut[1024] = { 0 };
		sprintf_s(szOut, "HideBrowser %x\n", hwnd);
		OutputDebugStringA(szOut);
#endif
	}
}

void BrowserWindowStdWin::Show() {
  REQUIRE_MAIN_THREAD();
  TabShow();
}

void BrowserWindowStdWin::Hide() {
  REQUIRE_MAIN_THREAD();
  TabHide();
}

void BrowserWindowStdWin::SetBounds(int x, int y, size_t width, size_t height) {
  REQUIRE_MAIN_THREAD();

  if (m_bIEKernel) {
	  if(ie_client_handler) ie_client_handler->UpdatePos(x, y, width, height,m_bTopFull);
  }
  else {
	  HWND hwnd = GetWindowHandle();
	  if(!m_bTabWindow) SetWindowPos(hwnd, NULL, x, y, static_cast<int>(width),static_cast<int>(height), SWP_NOZORDER);
	  else
	  {
		  if (!m_bTopFull)
		  {
			  if (!width && !height) {
				  //move
				  HWND hParent = GetParent(hwnd);
				  RECT rcParent = { 0 };
				  GetWindowRect(hParent, &rcParent);
				  int nX = x + rcParent.left;
				  int nY = y + rcParent.top;
				  SetWindowPos(hwnd, NULL, nX, nY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			  }
			  else {
				  HWND hParent = GetParent(hwnd);
				  RECT rcParent = { 0 };
				  GetWindowRect(hParent, &rcParent);
				  int nX = x + rcParent.left;
				  int nY = y + rcParent.top;
				  SetWindowPos(hwnd, NULL, nX, nY, width, height, SWP_NOZORDER);
			  }
		  }
	  }
  }
}

void BrowserWindowStdWin::SetFocus(bool focus) {
  REQUIRE_MAIN_THREAD();

  if (browser_)
    browser_->GetHost()->SetFocus(focus);
}


void BrowserWindowStdWin::SetMute(bool bMute)
{
    if (m_bIEKernel) {
		if (ie_client_handler) ie_client_handler->SetMute(bMute);
    }
    else {
        __super::SetMute(bMute);
    }
}


bool BrowserWindowStdWin::IsIEKernel()
{
    return m_bIEKernel;
}

bool BrowserWindowStdWin::IsShow()
{
	HWND hHostWnd = GetWindowHandle();
	return IsWindowVisible(hHostWnd);
}

bool BrowserWindowStdWin::IsMute()
{
	if (m_bIEKernel) {
		if (ie_client_handler) return ie_client_handler->IsMute();
		else return true;
	}
	else if(browser_){
		return browser_->GetHost()->IsAudioMuted();
	}
	else return true;
}

void PostCloseTask(BrowserWindow::Delegate* delgate,int nBrowserID) {
	if (delgate) delgate->OnBrowserWindowDestroyed(nBrowserID);
}

bool BrowserWindowStdWin::CloseBrowser()

{
    if (!m_bIEKernel)
    {
        browser_->GetHost()->CloseBrowser(false);
    }
    else if (ie_client_handler)
    {
        ie_client_handler->CloseBrowser();
        delete ie_client_handler;
		ie_client_handler = nullptr;
		is_closing_ = true;

		char szOut[256] = { 0 };
		sprintf_s(szOut, 256, "Browser:[%d] DoClose\n", nBrowserID);
		OutputDebugStringA(szOut);
		CefPostTask(TID_UI, base::Bind(PostCloseTask, delegate_ ,nBrowserID));
    }
    else return false;

    return true;
}

ClientWindowHandle BrowserWindowStdWin::GetWindowHandle() const {
  REQUIRE_MAIN_THREAD();

  if(!m_bIEKernel)
  {
	  if(!m_bTabWindow)
	  {
		  if (browser_)
			  return browser_->GetHost()->GetWindowHandle();
		  else return NULL;
	  }
	  else
	  {
		  return m_hTabHostWnd;
	  }
  }
  else {
      if (ie_client_handler)
          return ie_client_handler->GetHostWnd(m_bTopFull);
  }
  return NULL;
}


HWND BrowserWindowStdWin::GetBrowserHostWindowHandle()
{
	if (browser_)
		return browser_->GetHost()->GetWindowHandle();
	return NULL;
}

void BrowserWindowStdWin::ReleaseHostWndIfTab()
{
	if (m_hTabHostWnd && m_bTabWindow && IsWindow(m_hTabHostWnd))
	{
		DestroyWindow(m_hTabHostWnd);
		m_hTabHostWnd = NULL;
	}
}

void BrowserWindowStdWin::OnIETitleChange(std::string szTitle)
{
	if (IsSelected() && delegate_)
	{
		sCurTitle = UtilString::SA2U(szTitle);
		delegate_->OnSetTitle(sCurTitle);
	}
}

void BrowserWindowStdWin::OnIEProcessDeamon(int nNewBrowserID)
{
	nBrowserID = nNewBrowserID;
}

void BrowserWindowStdWin::SetTopFull(bool bTopFull)
{
	m_bTopFull = bTopFull;
}

void BrowserWindowStdWin::OnTabHostSizeChange(bool bMin)
{
	if(m_hTabHostWnd && m_bTabWindow)
	{
		RECT rcParent = { 0 };
		GetWindowRect(m_hTabHostWnd, &rcParent);
		HWND hwnd = GetBrowserHostWindowHandle();
		SetWindowPos(hwnd, NULL, 0, 0, rcParent.right-rcParent.left, rcParent.bottom-rcParent.top, SWP_NOZORDER);
	}
}

bool BrowserWindowStdWin::IsTabBrowser()
{
	return m_bTabWindow;
}

}  // namespace client
