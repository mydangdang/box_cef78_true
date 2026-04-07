// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_BROWSER_WINDOW_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_BROWSER_WINDOW_H_
#pragma once

#include "include/base/cef_scoped_ptr.h"
#include "include/cef_browser.h"
#include "tests/cefclient/browser/client_handler.h"
#include "tests/cefclient/browser/client_types.h"
#include "tests/cefclient/browser/IEBrowserHandler.h"

namespace client {

// Represents a native child window hosting a single browser instance. The
// methods of this class must be called on the main thread unless otherwise
// indicated.
class BrowserWindow : public ClientHandler::Delegate {
 public:
  // This interface is implemented by the owner of the BrowserWindow. The
  // methods of this class will be called on the main thread.
  class Delegate {
   public:
    // Called when the browser has been created.
    virtual void OnBrowserCreated(CefRefPtr<CefBrowser> browser,BrowserWindow *pBrowser) = 0;

    // Called when the BrowserWindow is closing.
    virtual void OnBrowserWindowClosing() {}

    // Called when the BrowserWindow has been destroyed.
    virtual void OnBrowserWindowDestroyed(int nBrowserID) = 0;

    virtual void OnBrowserNavigate(CefRefPtr<CefBrowser> browser, std::string sTargetTag, std::string szTargetURL) {}

    // Set the window URL address.
    virtual void OnSetAddress(const std::string& url) = 0;

    // Set the window title.
    virtual void OnSetTitle(const std::string& title) = 0;

    // Set fullscreen mode.
    virtual void OnSetFullscreen(bool fullscreen) = 0;

    // Auto-resize contents.
    virtual void OnAutoResize(const CefSize& new_size) = 0;

    // Set the loading state.
    virtual void OnSetLoadingState(bool isLoading,
                                   bool canGoBack,
                                   bool canGoForward) = 0;

    // Set the draggable regions.
    virtual void OnSetDraggableRegions(
        const std::vector<CefDraggableRegion>& regions) = 0;

   protected:
    virtual ~Delegate() {}
  };

  // Create a new browser and native window.
  virtual void CreateBrowser(ClientWindowHandle parent_handle,
                             const CefRect& rect,
                             const CefBrowserSettings& settings,
                             CefRefPtr<CefDictionaryValue> extra_info,
                             CefRefPtr<CefRequestContext> request_context) = 0;

  // Retrieve the configuration that will be used when creating a popup window.
  // The popup browser will initially be parented to |temp_handle| which should
  // be a pre-existing hidden window. The native window will be created later
  // after the browser has been created. This method will be called on the
  // browser process UI thread.
  virtual void GetPopupConfig(CefWindowHandle temp_handle,
                              CefWindowInfo& windowInfo,
                              CefRefPtr<CefClient>& client,
                              CefBrowserSettings& settings) = 0;

  // Show the popup window with correct parent and bounds in parent coordinates.
  virtual void ShowPopup(ClientWindowHandle parent_handle,
                         int x,
                         int y,
                         size_t width,
                         size_t height) = 0;

  virtual void TabShow() = 0;
  virtual void TabHide() = 0;

  // Show the window.
  virtual void Show() = 0;

  // Hide the window.
  virtual void Hide() = 0;

  // Set the window bounds in parent coordinates.
  virtual void SetBounds(int x, int y, size_t width, size_t height) = 0;

  // Set focus to the window.
  virtual void SetFocus(bool focus) = 0;

  // Set the device scale factor. Only used in combination with off-screen
  // rendering.
  virtual void SetDeviceScaleFactor(float device_scale_factor);

  // Returns the device scale factor. Only used in combination with off-screen
  // rendering.
  virtual float GetDeviceScaleFactor() const;

  // Returns the window handle.
  virtual ClientWindowHandle GetWindowHandle() const = 0;
  virtual LRESULT CALLBACK OsrWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { return DefWindowProc(hWnd, message, wParam, lParam); };

  virtual void SetMute(bool bMute);
  virtual bool IsIEKernel();
  virtual bool IsShow();
  virtual bool IsMute();
  virtual bool CloseBrowser();
  virtual void ReleaseHostWndIfTab();
  
  // Returns the browser owned by the window.
  CefRefPtr<CefBrowser> GetBrowser() const;

  // Returns true if the browser is closing.
  bool IsClosing() const;

  int GetBrowserID() {
      return nBrowserID;
  }
  void SetMarginRect(RECT rcMargin) {
      rcBoundMargin = rcMargin;
  }
  RECT GetMarginRect() {
      return rcBoundMargin;
  }
  void SetUniqeTag(const std::string& sTag) {
      sUqnieTag = sTag;
  }
  std::string GetUniqeTag() {
      return sUqnieTag;
  }
  void SetBrowserGroup(const std::string& sGroup) {
      sGroupTag = sGroup;
  }
  std::string GetBrowserGroup() {
      return sGroupTag;
  }
  std::string GetCurTitle() {
      return sCurTitle;
  }

  void SetSelected(bool bSelect) {
      bIsSelected = bSelect;
      if (bSelect) {
          OnSetTitle(sCurTitle);
      }
  }
  bool IsSelected() {
      return bIsSelected;
  }

  bool IsHookPopupLink() {
      return bHookPopupLink;
  }

  void SetHookPopupLink(bool bHook) {
      bHookPopupLink = bHook;
  }

  void SetJSObserFunc(CefRefPtr<CefMessageRouterBrowserSide::Callback> fCallBackJS) {
      m_fObservation = fCallBackJS;
  }
  CefRefPtr<CefMessageRouterBrowserSide::Callback> GetObserFunc() {
      return m_fObservation;
  }
  IEBrowserHandler* GetIEHandler() {
      return ie_client_handler;
  }
 
 protected:
  // Allow deletion via scoped_ptr only.
  friend struct base::DefaultDeleter<BrowserWindow>;

  // Constructor may be called on any thread.
  // |delegate| must outlive this object.
  explicit BrowserWindow(Delegate* delegate);

  // ClientHandler::Delegate methods.
  void OnBrowserCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
  void OnBrowserClosing(CefRefPtr<CefBrowser> browser) OVERRIDE;
  void OnBrowserClosed(CefRefPtr<CefBrowser> browser) OVERRIDE;
  void OnBrowserNavigate(CefRefPtr<CefBrowser> browser, std::string sTargetTag, std::string szTargetURL) OVERRIDE;
  void OnSetAddress(const std::string& url) OVERRIDE;
  void OnSetTitle(const std::string& title) OVERRIDE;
  void OnSetFullscreen(bool fullscreen) OVERRIDE;
  void OnAutoResize(const CefSize& new_size) OVERRIDE;
  void OnSetLoadingState(bool isLoading,
                         bool canGoBack,
                         bool canGoForward) OVERRIDE;
  void OnSetDraggableRegions(
      const std::vector<CefDraggableRegion>& regions) OVERRIDE;

  Delegate* delegate_;
  CefRefPtr<CefBrowser> browser_;
  CefRefPtr<ClientHandler> client_handler_;
  IEBrowserHandler* ie_client_handler;
  DWORD m_dwLastFree;
 
  bool is_closing_;
  RECT rcBoundMargin;
  bool bIsSelected;
  bool bStickToParent;
  bool bHookPopupLink;
  int nBrowserID;
  std::string sUqnieTag;
  std::string sGroupTag;
  std::string sCurTitle;
  CefRefPtr<CefMessageRouterBrowserSide::Callback> m_fObservation;
 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserWindow);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_BROWSER_WINDOW_H_
