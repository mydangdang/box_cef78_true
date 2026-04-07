// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_ROOT_WINDOW_WIN_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_ROOT_WINDOW_WIN_H_
#pragma once

#include <windows.h>

#include <commdlg.h>

#include <string>
#include "TopFullWnd.h"
#include "include/base/cef_scoped_ptr.h"
#include "tests/cefclient/browser/browser_window.h"
#include "tests/cefclient/browser/root_window.h"
#include "ToastWnd.h"

namespace client {

    enum LAST_WND_STATE {
        LWS_NOR = 1,
        LWS_MAX = 2,
        LWS_MIN = 3
    };

	struct tabState
	{
		int nBrowserID;
		bool bIE;
		bool bShow;
		bool bMute;
		bool bSelected;
		std::string szTagName;
		std::string szGroupName;
	};
// Windows implementation of a top-level native window in the browser process.
// The methods of this class must be called on the main thread unless otherwise
// indicated.
class RootWindowWin : public RootWindow,public ToastWnd::Delegate, public BrowserWindow::Delegate {
 public:
  // Constructor may be called on any thread.
  RootWindowWin();
  ~RootWindowWin();

  // RootWindow methods.
  void Init(RootWindow::Delegate* delegate,
            const RootWindowConfig& config,
            const CefBrowserSettings& settings) OVERRIDE;
  void InitAsPopup(RootWindow::Delegate* delegate,
                   bool with_controls,
                   bool with_osr,
                   const CefPopupFeatures& popupFeatures,
                   CefWindowInfo& windowInfo,
                   CefRefPtr<CefClient>& client,
                   CefBrowserSettings& settings) OVERRIDE;
  void InitAsTool(RootWindow::Delegate* delegate, 
      bool bOSR,
      bool bTopMost,
      bool bToolWnd,
	  const std::string& url,
	  const std::string& sTag, RECT rcPos, int nAction,
	  CefRefPtr<CefRequestContext> ctx) OVERRIDE;

  void Show(ShowMode mode) OVERRIDE;
  void Hide() OVERRIDE;
  void SetBounds(int x, int y, size_t width, size_t height) OVERRIDE;
  void Close(bool force) OVERRIDE;
  void SetDeviceScaleFactor(float device_scale_factor) OVERRIDE;
  float GetDeviceScaleFactor() const OVERRIDE;
  CefRefPtr<CefBrowser> GetBrowser() const OVERRIDE;
  ClientWindowHandle GetWindowHandle() const OVERRIDE;
  bool WithWindowlessRendering() const OVERRIDE;
  bool WithExtension() const OVERRIDE;
  bool IsToolWnd() OVERRIDE;
  bool IsOSR() OVERRIDE;

  void SetHookPopupLink(int nID, bool bHook) OVERRIDE;
  bool IsHookPopupLink(int nID) OVERRIDE;

  void OnToastEnd(int nCallerId, std::wstring szMsg);

  void BroadCastMsg(int nCallerID, const std::string& sCallerTag, CefRefPtr<CefDictionaryValue> msgJson,int nTargetType);
  void PostCustomMsg(int nCallerID, std::string& sTagName, const std::string& sMsgName, CefRefPtr<CefDictionaryValue> msgJson);

  bool OwnerBrowser(int nBrowserID);
  bool OwnerBrowser(const std::string& sTagName);

  CefRefPtr<CefBrowser> GetChildBrowser(int nBrowserID);
  CefRefPtr<CefBrowser> GetChildBrowser(const std::string& sTag);

  bool CreateTabBrowser(HWND hParent, RECT rcPos,bool bIE, int nFixFlash, bool bShareCache,bool bActive,bool bMuteOther,const std::string& url,const std::string& tag,const std::string& group);
  bool CreateToolBrowser(RECT rcPos,bool bOSR,bool bTopMost,bool bStandIco, bool bShareCache, const std::string& url, const std::string& sTag, int nKillFocusAction);

  void HideAllTabBrowser();
  bool ShowTabBrowser(int nBrowserID,bool bShow);
  bool ShowTabBrowser(const std::string& sUniqueTa, bool bShowg);
  bool MuteTabBrowser(int nBrowserID,bool bMute);
  bool MuteTabBrowser(const std::string& sUniqueTag, bool bMute);
  bool CloseTabBrowser(int nBrowserID);
  bool CloseTabBrowser(const std::string& sUniqueTag);
  bool SelectTabBrowser(const std::string& sUniqueTag, bool bMuteOthers);
  bool TabNavigate(const std::string& sUniqueTag, const std::string& sURL);
  bool TabReload(const std::string& sUniqueTag, bool bCache);
  bool ClearCache(const std::string& sUniqueTag);
  bool ShowTopFull(const std::string& sUniqueTag);
  bool GetTabStates(const std::string& sTagName, std::vector<tabState>& stats);
  bool IsBrowserShow(const std::string& sTagName);
  bool IsBrowserMute(const std::string& sTagName);
  
  void OnTopFullHotKey();
  void OnBossHotKey();

  void SetCallBackJSFunc(int nBrowserID,CefRefPtr<CefMessageRouterBrowserSide::Callback> callBackFunc);

  void EnableResize(bool bEnable);
  void SetMinSize(int nMinWidth, int nMinHeight);

  std::string GetBrowserTagFromID(int nBrowserID);

  int GetMainBrowserID();
  std::string GetMainBrowserTag();
  void ShowToast(int nCallerID,const std::wstring& szMsg, RECT rcPos, int nShowDuration, int nFadeDuration);
  bool IsUnderBoss();
 private:
  void CreateBrowserWindow(const std::string& startup_url);
  void CreateRootWindow(const CefBrowserSettings& settings,
                        bool initially_hidden);

  // Register the root window class.
  static void RegisterRootClass(HINSTANCE hInstance,
                                const std::wstring& window_class,
                                HBRUSH background_brush);


  // Window procedure for the find dialog.
  static LRESULT CALLBACK FindWndProc(HWND hWnd,
                                      UINT message,
                                      WPARAM wParam,
                                      LPARAM lParam);

  // Window procedure for the root window.
  static LRESULT CALLBACK RootWndProc(HWND hWnd,
                                      UINT message,
                                      WPARAM wParam,
                                      LPARAM lParam);

  // Event handlers.
  void OnPaint();
  void OnFocus();
  void OnActivate(bool active);
  void OnSize(bool minimized);
  void OnMinimize();
  void OnMove();
  void OnDpiChanged(WPARAM wParam, LPARAM lParam);
  bool OnEraseBkgnd();
  bool OnCommand(UINT id);
  void OnFind();
  void OnFindEvent();
  void OnNCCreate(LPCREATESTRUCT lpCreateStruct);
  void OnCreate(LPCREATESTRUCT lpCreateStruct);
  bool OnClose(int nID);
  void OnDestroyed();
  void OnLostFoucs();
  void OnActiveApp(bool bActive);
  void OnAppActived(bool bActive, HWND hWnd);

  // BrowserWindow::Delegate methods.
  void OnBrowserCreated(CefRefPtr<CefBrowser> browser, BrowserWindow *pBrowserWindow) OVERRIDE;
  void OnBrowserWindowDestroyed(int nBrowserID) OVERRIDE;
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

  void NotifyDestroyedIfDone();

  // After initialization all members are only accessed on the main thread.
  // Members set during initialization.
  int m_nKillFocusAction;
  int nMainBrowserID;
  bool isAsTool;
  bool isToolWnd;
  bool isDevsTool;
  bool with_controls_;
  bool always_on_top_;
  bool with_osr_;
  bool with_extension_;
  bool is_popup_;
  bool m_bResizable;
  SIZE m_sMinSize;
  RECT start_rect_;
  std::string szMainBrowserTag;
  std::map<std::string, std::string> mGroupLastTag;
  scoped_ptr<BrowserWindow> browser_window_;

  std::vector<BrowserWindow*> vChildBrowsers;

  bool m_bNeedResotreForBossKey;
  std::vector<BrowserWindow*> vBossHushedBrowser;

  CefBrowserSettings browser_settings_;
  bool initialized_;

  // Main window.
  HWND hwnd_;
  HWND hCurForgroundWnd;
  LAST_WND_STATE lastWndState;
  // Draggable region.
  HRGN draggable_region_;

  // Font for buttons and text fields.
  HFONT font_;
  int font_height_;

  // Buttons.
  HWND back_hwnd_;
  HWND forward_hwnd_;
  HWND reload_hwnd_;
  HWND stop_hwnd_;

  // URL text field.
  HWND edit_hwnd_;
  WNDPROC edit_wndproc_old_;

  // Find dialog.
  HWND find_hwnd_;
  UINT find_message_id_;
  WNDPROC find_wndproc_old_;

  // Find dialog state.
  FINDREPLACE find_state_;
  WCHAR find_buff_[80];
  std::wstring find_what_last_;
  bool find_next_;
  bool find_match_case_last_;

  bool window_destroyed_;
  bool browser_destroyed_;

  bool bIsBossIn;
  bool called_enable_non_client_dpi_scaling_;

  TopFullWnd m_FullTopManager;
  ToastWnd m_ToastWnd;

  DISALLOW_COPY_AND_ASSIGN(RootWindowWin);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_ROOT_WINDOW_WIN_H_
