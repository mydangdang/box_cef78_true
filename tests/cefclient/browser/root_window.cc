// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "tests/cefclient/browser/root_window.h"

#include "tests/cefclient/browser/main_context.h"
#include "tests/cefclient/browser/root_window_manager.h"

namespace client {

RootWindowConfig::RootWindowConfig()
    : always_on_top(false),
      with_controls(true),
      with_osr(false),
      with_extension(false),
      initially_hidden(false),
      url(MainContext::Get()->GetMainURL()) {}

RootWindow::RootWindow() : delegate_(NULL) {
    m_bHookPopupLink = false;
}

RootWindow::~RootWindow() {}

// static
scoped_refptr<RootWindow> RootWindow::GetForBrowser(int browser_id) {
  return MainContext::Get()->GetRootWindowManager()->GetWindowForBrowser(
      browser_id);
}

void RootWindow::OnTopFullHotKey()
{

}

void RootWindow::OnBossHotKey()
{

}

void RootWindow::OnAppActived(bool bActive, HWND hWnd)
{

}

void RootWindow::OnExtensionsChanged(const ExtensionSet& extensions) {
  REQUIRE_MAIN_THREAD();
  DCHECK(delegate_);
  DCHECK(!WithExtension());

  if (extensions.empty())
    return;

  ExtensionSet::const_iterator it = extensions.begin();
  for (; it != extensions.end(); ++it) {
    delegate_->CreateExtensionWindow(*it, CefRect(), NULL, base::Closure(),
                                     WithWindowlessRendering());
  }
}

void RootWindow::BroadCastMsg(int nCallerID, const std::string& sCallerTag, CefRefPtr<CefDictionaryValue> msgJson, int nTargetType)
{

}

void RootWindow::PostCustomMsg(int nCallerID, std::string& sTagName, const std::string& sMsgName, CefRefPtr<CefDictionaryValue> msgJson)
{

}

bool RootWindow::OwnerBrowser(int nBrowserID)
{
    return false;
}

bool RootWindow::OwnerBrowser(const std::string& sTagName)
{
	return false;
}

CefRefPtr<CefBrowser> RootWindow::GetChildBrowser(int nBrowserID)
{
    return nullptr;
}

CefRefPtr<CefBrowser> RootWindow::GetChildBrowser(const std::string& sTag)
{
    return nullptr;
}

std::string RootWindow::GetBrowserTagFromID(int nBrowserID)
{
    return "";
}

int RootWindow::GetMainBrowserID()
{
    return 0;
}

std::string RootWindow::GetMainBrowserTag()
{
    return "";
}

bool RootWindow::IsHookPopupLink(int nID)
{
    return m_bHookPopupLink;
}

void RootWindow::SetHookPopupLink(int nID,bool bHook)
{
    m_bHookPopupLink = bHook;
}

bool RootWindow::IsToolWnd()
{
    return false;
}

bool RootWindow::IsOSR()
{
    return false;
}

bool RootWindow::IsUnderBoss()
{
    return false;
}

}  // namespace client
