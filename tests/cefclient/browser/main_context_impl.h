// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_MAIN_CONTEXT_IMPL_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_MAIN_CONTEXT_IMPL_H_
#pragma once

#include "include/base/cef_scoped_ptr.h"
#include "include/base/cef_thread_checker.h"
#include "include/cef_app.h"
#include "include/cef_command_line.h"
#include "tests/cefclient/browser/main_context.h"
#include "tests/cefclient/browser/root_window_manager.h"
#include "../ShutDownPlan.h"
#include "../HotKeyManager.h"
#include "CBaseTrayIcon.h"

namespace client {

// Used to store global context in the browser process.
class MainContextImpl : public MainContext ,public ShutDownPlan::Delegate,public HotKeyObserver {
 public:
  MainContextImpl(CefRefPtr<CefCommandLine> command_line,
                  bool terminate_when_all_windows_closed,std::string szCfg);

  // MainContext members.
  std::string GetConsoleLogPath() OVERRIDE;
  std::string GetDownloadPath(const std::string& file_name) OVERRIDE;
  std::string GetAppWorkingDirectory() OVERRIDE;
  std::string GetMainURL() OVERRIDE;

  void UpdateMemStorage(const std::string& sTag, const std::string& sValue) OVERRIDE;
  std::string GetMemStorage(const std::string& sTag) OVERRIDE;

  std::string GetVersion() OVERRIDE;
  void SetUID(std::string szUID);
  std::string GetBaseInfo() OVERRIDE;
  std::wstring GetHardwarInfo() OVERRIDE;
  void SetInfo(const std::string& szSID, const std::string& szCID, const std::string& szGID) OVERRIDE;
  std::string GetInitFrameName() OVERRIDE;
  RECT GetInitPos() OVERRIDE;

  cef_color_t GetBackgroundColor() OVERRIDE;
  bool UseViews() OVERRIDE;
  bool UseWindowlessRendering() OVERRIDE;
  bool TouchEventsEnabled() OVERRIDE;
  void PopulateSettings(CefSettings* settings) OVERRIDE;
  void PopulateBrowserSettings(CefBrowserSettings* settings) OVERRIDE;
  void PopulateOsrSettings(OsrRendererSettings* settings) OVERRIDE;
  RootWindowManager* GetRootWindowManager() OVERRIDE;
  bool SysShutDownPlan(int nBrowserID,bool bAdd, int nLeftMinites, int nWarninigMinites) OVERRIDE;

  void OnPlanWarning(int nBrowserID,int nLeftMinites) OVERRIDE;
  std::string GetClockName() OVERRIDE;
  void OnClock(std::string szName) OVERRIDE;
  void OnRecordScreenStop(std::string szTagName) OVERRIDE;

  // Initialize CEF and associated main context state. This method must be
  // called on the same thread that created this object.
  bool Initialize(const CefMainArgs& args,
                  const CefSettings& settings,
                  CefRefPtr<CefApp> application,
                  void* windows_sandbox_info);

  // Shut down CEF and associated context state. This method must be called on
  // the same thread that created this object.
  void Shutdown();
  void OnHotKey(stHotKey* pHotKey);
  void CreateTrayICON(HINSTANCE hInst,HWND hWnd);
  void ShowBalloonTip(std::wstring szTip);
  
  HMODULE GetPlugInDLL();

  std::string main_url_;
  HMODULE m_hPluginDLL = NULL;
  HANDLE m_hPluginEvent = NULL;

  std::string m_szSID;
  std::string m_szCPUID;
  std::string m_szMAC;
  std::string m_szHost;
  std::string m_szShareData;
  bool m_bEnableADS = true;
 private:
  // Allow deletion via scoped_ptr only.
  friend struct base::DefaultDeleter<MainContextImpl>;

  ~MainContextImpl();

  // Returns true if the context is in a valid state (initialized and not yet
  // shut down).
  bool InValidState() const { return initialized_ && !shutdown_; }

  CefRefPtr<CefCommandLine> command_line_;
  const bool terminate_when_all_windows_closed_;

  // Track context state. Accessing these variables from multiple threads is
  // safe because only a single thread will exist at the time that they're set
  // (during context initialization and shutdown).
  bool initialized_;
  bool shutdown_;
 
  std::string m_szClockName;
  std::string m_szBaseInfo;
  std::wstring m_szHardwarInfo;
  std::string m_szInitFrameName;

  cef_color_t background_color_;
  cef_color_t browser_background_color_;
  bool use_windowless_rendering_;
  int windowless_frame_rate_;
  bool use_views_;
  bool touch_events_enabled_;
 
  scoped_ptr<RootWindowManager> root_window_manager_;

  std::map<std::string, std::string> m_JsMemStorage;
#if defined(OS_WIN)
  bool shared_texture_enabled_;
#endif

  bool external_begin_frame_enabled_;

  RECT m_InitPos;
  CBaseTrayIcon m_oBaseTrayIcon;
  // Used to verify that methods are called on the correct thread.
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(MainContextImpl);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_MAIN_CONTEXT_IMPL_H_
