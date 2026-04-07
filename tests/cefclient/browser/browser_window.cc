// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "tests/cefclient/browser/browser_window.h"

#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"
#include "tests/shared/browser/main_message_loop.h"
#include "../MemoryDeal.h"


namespace client {

BrowserWindow::BrowserWindow(Delegate* delegate)
    : delegate_(delegate), is_closing_(false) {
    nBrowserID = 0;
    m_fObservation = NULL;
    ie_client_handler = NULL;
	bIsSelected = true;
	bStickToParent = false;
    bHookPopupLink = false;
  DCHECK(delegate_);
}

void BrowserWindow::SetDeviceScaleFactor(float device_scale_factor) {}

float BrowserWindow::GetDeviceScaleFactor() const {
  return 1.0f;
}


void BrowserWindow::SetMute(bool bMute)
{
    if (browser_ != NULL) {
        browser_->GetHost()->SetAudioMuted(bMute);
    }
}


bool BrowserWindow::IsIEKernel()
{
    return false;
}


bool BrowserWindow::IsShow()
{
	return true;
}

bool BrowserWindow::CloseBrowser()
{
    return true;
}

void BrowserWindow::ReleaseHostWndIfTab()
{

}

CefRefPtr<CefBrowser> BrowserWindow::GetBrowser() const {
  REQUIRE_MAIN_THREAD();
  return browser_;
}

bool BrowserWindow::IsClosing() const {
  REQUIRE_MAIN_THREAD();
  return is_closing_;
}

void GC(BrowserWindow* pWindow)
{
	if (!MemoryDealer.HasFreeOBJ(int(pWindow))) return;
	if (pWindow)
	{
		CefRefPtr<CefBrowser> browser = pWindow->GetBrowser();
		CefRefPtr<CefFrame> frame = browser->GetMainFrame();
		if (frame)
			frame->ExecuteJavaScript("console.log(window.gc);", frame->GetURL(), 0);
	}
	CefPostDelayedTask(TID_UI, base::Bind(&GC, pWindow), 1000*60*10);
}

void BrowserWindow::OnBrowserCreated(CefRefPtr<CefBrowser> browser) {
  REQUIRE_MAIN_THREAD();
  DCHECK(!browser_);
  browser_ = browser;
  nBrowserID = browser->GetIdentifier();
  if (sUqnieTag.empty()) {
	  sUqnieTag = browser->GetMainFrame()->GetName();
  }
  delegate_->OnBrowserCreated(browser,0);

  MemoryDealer.AddFreeOBJ(int(this));
  CefPostDelayedTask(TID_UI, base::Bind(&GC, this), 1000 * 60 * 10);
}

void BrowserWindow::OnBrowserClosing(CefRefPtr<CefBrowser> browser) {
  REQUIRE_MAIN_THREAD();
  DCHECK_EQ(browser->GetIdentifier(), browser_->GetIdentifier());

  MemoryDealer.DelFreeOBJ(int(this));

  is_closing_ = true;
  delegate_->OnBrowserWindowClosing();
}

void BrowserWindow::OnBrowserClosed(CefRefPtr<CefBrowser> browser) {
  REQUIRE_MAIN_THREAD();
  if (browser_.get()) {
    DCHECK_EQ(browser->GetIdentifier(), browser_->GetIdentifier());
    browser_ = NULL;
  }

  client_handler_->DetachDelegate();
  client_handler_ = NULL;

  // |this| may be deleted.
  delegate_->OnBrowserWindowDestroyed(browser->GetIdentifier());
}

void BrowserWindow::OnBrowserNavigate(CefRefPtr<CefBrowser> browser, std::string sTargetTag, std::string szTargetURL)
{
	REQUIRE_MAIN_THREAD();
	delegate_->OnBrowserNavigate(browser, sTargetTag,szTargetURL);
}

void BrowserWindow::OnSetAddress(const std::string& url) {
  REQUIRE_MAIN_THREAD();
  delegate_->OnSetAddress(url);
}

void BrowserWindow::OnSetTitle(const std::string& title) {
  REQUIRE_MAIN_THREAD();
  sCurTitle = title;
  delegate_->OnSetTitle(sCurTitle);
}

void BrowserWindow::OnSetFullscreen(bool fullscreen) {
  REQUIRE_MAIN_THREAD();
  delegate_->OnSetFullscreen(fullscreen);
}

void BrowserWindow::OnAutoResize(const CefSize& new_size) {
  REQUIRE_MAIN_THREAD();
  delegate_->OnAutoResize(new_size);
}

void BrowserWindow::OnSetLoadingState(bool isLoading,
                                      bool canGoBack,
                                      bool canGoForward) {
  REQUIRE_MAIN_THREAD();
  delegate_->OnSetLoadingState(isLoading, canGoBack, canGoForward);
}

void BrowserWindow::OnSetDraggableRegions(
    const std::vector<CefDraggableRegion>& regions) {
  REQUIRE_MAIN_THREAD();
  delegate_->OnSetDraggableRegions(regions);
}

bool BrowserWindow::IsMute()
{
	return false;
}

}  // namespace client
