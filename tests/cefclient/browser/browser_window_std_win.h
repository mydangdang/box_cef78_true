// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_BROWSER_WINDOW_STD_WIN_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_BROWSER_WINDOW_STD_WIN_H_
#pragma once

#include "tests/cefclient/browser/browser_window.h"

namespace client {

// Represents a native child window hosting a single windowed browser instance.
// The methods of this class must be called on the main thread unless otherwise
// indicated.
class BrowserWindowStdWin : public BrowserWindow , public IEBrowserHandler::IEDelegate {
 public:
  // Constructor may be called on any thread.
  // |delegate| must outlive this object.
  BrowserWindowStdWin(Delegate* delegate, const std::string& startup_url,const std::string& tag,const std::string& group,bool bIE,int nFixFlash = 0,bool bTab = false);
  // BrowserWindow methods.
  void CreateBrowser(ClientWindowHandle parent_handle,
                     const CefRect& rect,
                     const CefBrowserSettings& settings,
                     CefRefPtr<CefDictionaryValue> extra_info,
                     CefRefPtr<CefRequestContext> request_context) OVERRIDE;
  void GetPopupConfig(CefWindowHandle temp_handle,
                      CefWindowInfo& windowInfo,
                      CefRefPtr<CefClient>& client,
                      CefBrowserSettings& settings) OVERRIDE;
  void ShowPopup(ClientWindowHandle parent_handle,
                 int x,
                 int y,
                 size_t width,
                 size_t height) OVERRIDE;
  void TabShow() OVERRIDE;
  void TabHide() OVERRIDE;

  void Show() OVERRIDE;
  void Hide() OVERRIDE;
  void SetBounds(int x, int y, size_t width, size_t height) OVERRIDE;
  void SetFocus(bool focus) OVERRIDE;
  void SetMute(bool bMute) OVERRIDE;
  bool IsIEKernel() OVERRIDE;
  bool IsShow() OVERRIDE;
  bool IsMute() OVERRIDE;
  bool CloseBrowser()OVERRIDE;
  ClientWindowHandle GetWindowHandle() const OVERRIDE;

  HWND GetBrowserHostWindowHandle();
  void ReleaseHostWndIfTab();
  void OnIETitleChange(std::string szTitle);
  void OnIEProcessDeamon(int nNewBrowserID);
  void SetTopFull(bool bTopFull);
  void OnTabHostSizeChange(bool bMin);
  bool IsTabBrowser();
 private:
	 bool m_bTabWindow;
	 bool m_bTopFull;
     bool m_bIEKernel;
     RECT m_rcOld;

     HWND m_hTabHostWnd;
  DISALLOW_COPY_AND_ASSIGN(BrowserWindowStdWin);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_BROWSER_WINDOW_STD_WIN_H_
