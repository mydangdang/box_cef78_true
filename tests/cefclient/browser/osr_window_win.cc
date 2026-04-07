// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "tests/cefclient/browser/osr_window_win.h"

#include <windowsx.h>
#if defined(CEF_USE_ATL)
#include <oleacc.h>
#endif

#include "include/base/cef_build.h"
#include "tests/cefclient/browser/main_context.h"
#include "tests/cefclient/browser/osr_accessibility_helper.h"
#include "tests/cefclient/browser/osr_accessibility_node.h"
#include "tests/cefclient/browser/osr_ime_handler_win.h"
#include "tests/cefclient/browser/osr_render_handler_win_d3d11.h"
#include "tests/cefclient/browser/osr_render_handler_win_gl.h"
#include "tests/cefclient/browser/resource.h"
#include "tests/shared/browser/geometry_util.h"
#include "tests/shared/browser/main_message_loop.h"
#include "tests/shared/browser/util_win.h"
#include "GdiPlusEnv.h"
#include <commctrl.h>
#include <minwinbase.h>

namespace client {

namespace {

const wchar_t kWndClass[] = L"Client_OsrWindow";

// Helper funtion to check if it is Windows8 or greater.
// https://msdn.microsoft.com/en-us/library/ms724833(v=vs.85).aspx
inline BOOL IsWindows_8_Or_Newer() {
  OSVERSIONINFOEX osvi = {0};
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  osvi.dwMajorVersion = 6;
  osvi.dwMinorVersion = 2;
  DWORDLONG dwlConditionMask = 0;
  VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
  VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
  return ::VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION,
                             dwlConditionMask);
}

// Helper function to detect mouse messages coming from emulation of touch
// events. These should be ignored.
bool IsMouseEventFromTouch(UINT message) {
#define MOUSEEVENTF_FROMTOUCH 0xFF515700
  return (message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST) &&
         (GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) ==
             MOUSEEVENTF_FROMTOUCH;
}

class CreateBrowserHelper {
 public:
  CreateBrowserHelper(HWND hwnd,
                      const RECT& rect,
                      CefRefPtr<CefClient> handler,
                      const std::string& url,
                      const CefBrowserSettings& settings,
                      CefRefPtr<CefDictionaryValue> extra_info,
                      CefRefPtr<CefRequestContext> request_context,
                      OsrWindowWin* osr_window_win)
      : hwnd_(hwnd),
        rect_(rect),
        handler_(handler),
        url_(url),
        settings_(settings),
        extra_info_(extra_info),
        request_context_(request_context),
        osr_window_win_(osr_window_win) {}

  HWND hwnd_;
  RECT rect_;
  CefRefPtr<CefClient> handler_;
  std::string url_;
  CefBrowserSettings settings_;
  CefRefPtr<CefDictionaryValue> extra_info_;
  CefRefPtr<CefRequestContext> request_context_;
  OsrWindowWin* osr_window_win_;
};

}  // namespace

OsrWindowWin::OsrWindowWin(Delegate* delegate,
                           const OsrRendererSettings& settings)
    : delegate_(delegate),
      hwnd_(NULL),
      device_scale_factor_(0),
      hidden_(false),
      last_mouse_pos_(),
      current_mouse_pos_(),
      mouse_rotation_(false),
      mouse_tracking_(false),
      last_click_x_(0),
      last_click_y_(0),
      last_click_button_(MBT_LEFT),
      last_click_count_(0),
      last_click_time_(0),
      last_mouse_down_on_view_(false) {
  DCHECK(delegate_);
  browser_ = NULL;
  client_rect_ = {0};
  m_hEraser = ::CreateSolidBrush(RGB(0, 0, 0));

}

OsrWindowWin::~OsrWindowWin() {
  CEF_REQUIRE_UI_THREAD();
  // The native window should have already been destroyed.
  if (m_pOSRBitmap)
  {
	  delete m_pOSRBitmap;
	  m_pOSRBitmap = NULL;
  }
  if (m_pPopBitmap)
  {
      delete m_pPopBitmap;
      m_pPopBitmap = NULL;
  }
  if (m_hDcOffScreen)
  {
	  if (m_hBmpOld)
		  ::SelectObject(m_hDcOffScreen, m_hBmpOld);
	  DeleteDC(m_hDcOffScreen);
	  m_hDcOffScreen = NULL;
  }
  if (m_hBmpOffScreen)
  {
	  DeleteObject(m_hBmpOffScreen);
	  m_hBmpOffScreen = NULL;
  }
  if (m_hEraser)
  {
	  DeleteObject(m_hEraser);
	  m_hEraser = NULL;
  }
}

void CreateBrowserWithHelper(CreateBrowserHelper* helper) {
  helper->osr_window_win_->CreateBrowser(
      helper->hwnd_, helper->rect_, helper->handler_, helper->settings_,
      helper->extra_info_, helper->request_context_, helper->url_);
  delete helper;
}

void OsrWindowWin::CreateBrowser(HWND parent_hwnd,
                                 const RECT& rect,
                                 CefRefPtr<CefClient> handler,
                                 const CefBrowserSettings& settings,
                                 CefRefPtr<CefDictionaryValue> extra_info,
                                 CefRefPtr<CefRequestContext> request_context,
                                 const std::string& startup_url) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CreateBrowserHelper* helper =
        new CreateBrowserHelper(parent_hwnd, rect, handler, startup_url,
                                settings, extra_info, request_context, this);
    CefPostTask(TID_UI, base::Bind(CreateBrowserWithHelper, helper));
    return;
  }

  hwnd_ = parent_hwnd;

  ime_handler_.reset(new OsrImeHandlerWin(hwnd_));
  if (client::MainContext::Get()->TouchEventsEnabled())
	  RegisterTouchWindow(hwnd_, 0);
  NotifyNativeWindowCreated(hwnd_);

  CefWindowInfo window_info;
  window_info.SetAsWindowless(hwnd_);

  if (GetWindowLongPtr(parent_hwnd, GWL_EXSTYLE) & WS_EX_NOACTIVATE) {
    // Don't activate the browser window on creation.
    window_info.ex_style |= WS_EX_NOACTIVATE;
  }

  // Create the browser asynchronously.
  CefBrowserHost::CreateBrowser(window_info, handler, startup_url, settings,
                                extra_info, request_context);
}

void OsrWindowWin::ShowPopup(HWND parent_hwnd,
                             int x,
                             int y,
                             size_t width,
                             size_t height) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::ShowPopup, this, parent_hwnd,
                                   x, y, width, height));
    return;
  }

  DCHECK(browser_.get());

  // Create the native window.
  const RECT rect = {x, y, x + static_cast<int>(width),
                     y + static_cast<int>(height)};
  

  // Send resize notification so the compositor is assigned the correct
  // viewport size and begins rendering.
  browser_->GetHost()->WasResized();

  Show();
}

void OsrWindowWin::Show() {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::Show, this));
    return;
  }

  if (!browser_)
    return;

  if (hidden_) {
    // Set the browser as visible.
    browser_->GetHost()->WasHidden(false);
    hidden_ = false;
  }

  // Give focus to the browser.
  browser_->GetHost()->SendFocusEvent(true);
}

void OsrWindowWin::Hide() {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::Hide, this));
    return;
  }

  if (!browser_)
    return;

  // Remove focus from the browser.
  browser_->GetHost()->SendFocusEvent(false);

  if (!hidden_) {
    // Set the browser as hidden.
    browser_->GetHost()->WasHidden(true);
    hidden_ = true;
  }
}

void OsrWindowWin::SetBounds(int x, int y, size_t width, size_t height) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::SetBounds, this, x, y, width,
                                   height));
    return;
  }

//   client_rect_.left = x;
//   client_rect_.top = y;
//   client_rect_.right = x + width;
//   client_rect_.bottom = y + height;
// 
//   char szOut[1024] = { 0 };
//   sprintf_s(szOut, "SetBounds %d %d \n", client_rect_.right- client_rect_.left, client_rect_.bottom-client_rect_.top);
//   OutputDebugStringA(szOut);
// 
//   OnSize();
}

void OsrWindowWin::SetFocus() {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::SetFocus, this));
    return;
  }
  
  if (hwnd_) {
    // Give focus to the native window.
    ::SetFocus(hwnd_);
  }
}

void OsrWindowWin::SetDeviceScaleFactor(float device_scale_factor) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::SetDeviceScaleFactor, this,
                                   device_scale_factor));
    return;
  }

  if (device_scale_factor == device_scale_factor_)
    return;

  device_scale_factor_ = device_scale_factor;
  if (browser_) {
    browser_->GetHost()->NotifyScreenInfoChanged();
    browser_->GetHost()->WasResized();
  }
}

void OsrWindowWin::NotifyNativeWindowCreated(HWND hwnd) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&OsrWindowWin::NotifyNativeWindowCreated, this, hwnd));
    return;
  }

  delegate_->OnOsrNativeWindowCreated(hwnd);
}

void OsrWindowWin::OnIMESetContext(UINT message, WPARAM wParam, LPARAM lParam) {
  // We handle the IME Composition Window ourselves (but let the IME Candidates
  // Window be handled by IME through DefWindowProc()), so clear the
  // ISC_SHOWUICOMPOSITIONWINDOW flag:


  lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
  ::DefWindowProc(hwnd_, message, wParam, lParam);

  // Create Caret Window if required
  if (ime_handler_) {
    ime_handler_->CreateImeWindow();
    ime_handler_->MoveImeWindow();
  }
}

void OsrWindowWin::OnIMEStartComposition() {
 
  if (ime_handler_) {
    ime_handler_->CreateImeWindow();
    ime_handler_->MoveImeWindow();
    ime_handler_->ResetComposition();
  }
}

void OsrWindowWin::OnIMEComposition(UINT message,
                                    WPARAM wParam,
                                    LPARAM lParam) {
 
  if (browser_ && ime_handler_) {
    CefString cTextStr;
    if (ime_handler_->GetResult(lParam, cTextStr)) {
      // Send the text to the browser. The |replacement_range| and
      // |relative_cursor_pos| params are not used on Windows, so provide
      // default invalid values.
      browser_->GetHost()->ImeCommitText(cTextStr,
                                         CefRange(UINT32_MAX, UINT32_MAX), 0);
      ime_handler_->ResetComposition();
      // Continue reading the composition string - Japanese IMEs send both
      // GCS_RESULTSTR and GCS_COMPSTR.
    }

    std::vector<CefCompositionUnderline> underlines;
    int composition_start = 0;

    if (ime_handler_->GetComposition(lParam, cTextStr, underlines,
                                     composition_start)) {
      // Send the composition string to the browser. The |replacement_range|
      // param is not used on Windows, so provide a default invalid value.
      browser_->GetHost()->ImeSetComposition(
          cTextStr, underlines, CefRange(UINT32_MAX, UINT32_MAX),
          CefRange(composition_start,
                   static_cast<int>(composition_start + cTextStr.length())));

      // Update the Candidate Window position. The cursor is at the end so
      // subtract 1. This is safe because IMM32 does not support non-zero-width
      // in a composition. Also,  negative values are safely ignored in
      // MoveImeWindow
      ime_handler_->UpdateCaretPosition(composition_start - 1);
    } else {
      OnIMECancelCompositionEvent();
    }
  }
}

void OsrWindowWin::OnIMECancelCompositionEvent() {

  if (browser_ && ime_handler_) {
    browser_->GetHost()->ImeCancelComposition();
    ime_handler_->ResetComposition();
    ime_handler_->DestroyImeWindow();
  }
}

// static
LRESULT CALLBACK OsrWindowWin::OsrWndProc(HWND hWnd,
                                          UINT message,
                                          WPARAM wParam,
                                          LPARAM lParam) {
  CEF_REQUIRE_UI_THREAD();


  // We want to handle IME events before the OS does any default handling.
  switch (message) {
    case WM_IME_SETCONTEXT:
      OnIMESetContext(message, wParam, lParam);
      return 0;
    case WM_IME_STARTCOMPOSITION:
      OnIMEStartComposition();
      return 0;
    case WM_IME_COMPOSITION:
      OnIMEComposition(message, wParam, lParam);
      return 0;
    case WM_IME_ENDCOMPOSITION:
      OnIMECancelCompositionEvent();
      // Let WTL call::DefWindowProc() and release its resources.
      break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_MOUSELEAVE:
    case WM_MOUSEWHEEL:
      OnMouseEvent(message, wParam, lParam);
      break;

    case WM_SIZE:
      OnSize();
      break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
      OnFocus(message == WM_SETFOCUS);
      break;

    case WM_CAPTURECHANGED:
    case WM_CANCELMODE:
      OnCaptureLost();
      break;

    case WM_SYSCHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    {
		if (message == WM_KEYDOWN && wParam == VK_PROCESSKEY)
		{
			HIMC imc = ::ImmGetContext(hwnd_);
            if (m_nIEMX <= 0 || m_nIEMY <= 0)
            {
                RECT rcPos = { 0 };
                GetWindowRect(hwnd_, &rcPos);
                m_nIEMX = rcPos.right - 100;
                m_nIEMY = rcPos.bottom - 100;
            }
			COMPOSITIONFORM candidate_position = { CFS_POINT, {m_nIEMX, m_nIEMY + 30 * GetDeviceScaleFactor()}, {0, 0, 0, 0} };
			::ImmSetCompositionWindow(imc, &candidate_position);
		}
        OnKeyEvent(message, wParam, lParam);
    }
      break;

    case WM_PAINT:
      OnPaint();
      return 0;

    case WM_ERASEBKGND:
      if (OnEraseBkgnd())
        break;
      // Don't erase the background.
      return 0;

    // If your application does not require Win7 support, please do consider
    // using WM_POINTER* messages instead of WM_TOUCH. WM_POINTER are more
    // intutive, complete and simpler to code.
    // https://msdn.microsoft.com/en-us/library/hh454903(v=vs.85).aspx
    case WM_TOUCH:
      if (OnTouchEvent(message, wParam, lParam))
        return 0;
      break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

void OsrWindowWin::OnMouseEvent(UINT message, WPARAM wParam, LPARAM lParam) {
  if (IsMouseEventFromTouch(message))
    return;

  CefRefPtr<CefBrowserHost> browser_host;
  if (browser_)
    browser_host = browser_->GetHost();

  LONG currentTime = 0;
  bool cancelPreviousClick = false;

  if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
      message == WM_MBUTTONDOWN || message == WM_MOUSEMOVE ||
      message == WM_MOUSELEAVE) {
    currentTime = GetMessageTime();
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);
    cancelPreviousClick =
        (abs(last_click_x_ - x) > (GetSystemMetrics(SM_CXDOUBLECLK) / 2)) ||
        (abs(last_click_y_ - y) > (GetSystemMetrics(SM_CYDOUBLECLK) / 2)) ||
        ((currentTime - last_click_time_) > GetDoubleClickTime());
    if (cancelPreviousClick &&
        (message == WM_MOUSEMOVE || message == WM_MOUSELEAVE)) {
      last_click_count_ = 0;
      last_click_x_ = 0;
      last_click_y_ = 0;
      last_click_time_ = 0;
    }
  }

  switch (message) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN: {
      ::SetCapture(hwnd_);
      ::SetFocus(hwnd_);
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);
      if (wParam & MK_SHIFT) {
        // Start rotation effect.
        last_mouse_pos_.x = current_mouse_pos_.x = x;
        last_mouse_pos_.y = current_mouse_pos_.y = y;
        mouse_rotation_ = true;
      } else {
        CefBrowserHost::MouseButtonType btnType =
            (message == WM_LBUTTONDOWN
                 ? MBT_LEFT
                 : (message == WM_RBUTTONDOWN ? MBT_RIGHT : MBT_MIDDLE));
        if (!cancelPreviousClick && (btnType == last_click_button_)) {
          ++last_click_count_;
        } else {
          last_click_count_ = 1;
          last_click_x_ = x;
          last_click_y_ = y;
        }
        last_click_time_ = currentTime;
        last_click_button_ = btnType;

        if (browser_host) {
          CefMouseEvent mouse_event;
          mouse_event.x = x;
          mouse_event.y = y;
          last_mouse_down_on_view_ = !IsOverPopupWidget(x, y);
          ApplyPopupOffset(mouse_event.x, mouse_event.y);
          DeviceToLogical(mouse_event, device_scale_factor_);
          mouse_event.modifiers = GetCefMouseModifiers(wParam);
          browser_host->SendMouseClickEvent(mouse_event, btnType, false,
                                            last_click_count_);
        }
      }
    } break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
      if (GetCapture() == hwnd_)
        ReleaseCapture();
      if (mouse_rotation_) {
        // End rotation effect.
        mouse_rotation_ = false;
      } else {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        CefBrowserHost::MouseButtonType btnType =
            (message == WM_LBUTTONUP
                 ? MBT_LEFT
                 : (message == WM_RBUTTONUP ? MBT_RIGHT : MBT_MIDDLE));
        if (browser_host) {
          CefMouseEvent mouse_event;
          mouse_event.x = x;
          mouse_event.y = y;
          if (last_mouse_down_on_view_ && IsOverPopupWidget(x, y) &&
              (GetPopupXOffset() || GetPopupYOffset())) {
            break;
          }
          ApplyPopupOffset(mouse_event.x, mouse_event.y);
          DeviceToLogical(mouse_event, device_scale_factor_);
          mouse_event.modifiers = GetCefMouseModifiers(wParam);
          browser_host->SendMouseClickEvent(mouse_event, btnType, true,
                                            last_click_count_);
        }
      }
      break;

    case WM_MOUSEMOVE: {
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);
      if (mouse_rotation_) {
        // Apply rotation effect.
        current_mouse_pos_.x = x;
        current_mouse_pos_.y = y;
        last_mouse_pos_.x = current_mouse_pos_.x;
        last_mouse_pos_.y = current_mouse_pos_.y;
      } else {
        if (!mouse_tracking_) {
          // Start tracking mouse leave. Required for the WM_MOUSELEAVE event to
          // be generated.
          TRACKMOUSEEVENT tme;
          tme.cbSize = sizeof(TRACKMOUSEEVENT);
          tme.dwFlags = TME_LEAVE;
          tme.hwndTrack = hwnd_;
          TrackMouseEvent(&tme);
          mouse_tracking_ = true;
        }

        if (browser_host) {
          CefMouseEvent mouse_event;
          mouse_event.x = x;
          mouse_event.y = y;
          ApplyPopupOffset(mouse_event.x, mouse_event.y);
          DeviceToLogical(mouse_event, device_scale_factor_);
          mouse_event.modifiers = GetCefMouseModifiers(wParam);
          browser_host->SendMouseMoveEvent(mouse_event, false);
        }
      }
      break;
    }

    case WM_MOUSELEAVE: {
      if (mouse_tracking_) {
        // Stop tracking mouse leave.
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE & TME_CANCEL;
        tme.hwndTrack = hwnd_;
        TrackMouseEvent(&tme);
        mouse_tracking_ = false;
      }

      if (browser_host) {
        // Determine the cursor position in screen coordinates.
        POINT p;
        ::GetCursorPos(&p);
        ::ScreenToClient(hwnd_, &p);

        CefMouseEvent mouse_event;
        mouse_event.x = p.x;
        mouse_event.y = p.y;
        DeviceToLogical(mouse_event, device_scale_factor_);
        mouse_event.modifiers = GetCefMouseModifiers(wParam);
        browser_host->SendMouseMoveEvent(mouse_event, true);
      }
    } break;

    case WM_MOUSEWHEEL:
      if (browser_host) {
        POINT screen_point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        HWND scrolled_wnd = ::WindowFromPoint(screen_point);
        if (scrolled_wnd != hwnd_)
          break;

        ScreenToClient(hwnd_, &screen_point);
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);

        CefMouseEvent mouse_event;
        mouse_event.x = screen_point.x;
        mouse_event.y = screen_point.y;
        ApplyPopupOffset(mouse_event.x, mouse_event.y);
        DeviceToLogical(mouse_event, device_scale_factor_);
        mouse_event.modifiers = GetCefMouseModifiers(wParam);
        browser_host->SendMouseWheelEvent(mouse_event,
                                          IsKeyDown(VK_SHIFT) ? delta : 0,
                                          !IsKeyDown(VK_SHIFT) ? delta : 0);
      }
      break;
  }
}

void OsrWindowWin::OnSize() {

	::GetClientRect(hwnd_, &client_rect_);

	if (browser_)
		browser_->GetHost()->WasResized();
}

void OsrWindowWin::OnFocus(bool setFocus) {
    if (setFocus)SetFocus();
  if (browser_)
    browser_->GetHost()->SendFocusEvent(setFocus);
}

void OsrWindowWin::OnCaptureLost() {
  if (mouse_rotation_)
    return;

  if (browser_)
    browser_->GetHost()->SendCaptureLostEvent();
}

void OsrWindowWin::OnKeyEvent(UINT message, WPARAM wParam, LPARAM lParam) {
  if (!browser_)
    return;
 
  CefKeyEvent event;
  event.windows_key_code = wParam;
  event.native_key_code = lParam;
  event.is_system_key = message == WM_SYSCHAR || message == WM_SYSKEYDOWN ||
                        message == WM_SYSKEYUP;

  if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
    event.type = KEYEVENT_RAWKEYDOWN;
  else if (message == WM_KEYUP || message == WM_SYSKEYUP)
    event.type = KEYEVENT_KEYUP;
  else
    event.type = KEYEVENT_CHAR;
  event.modifiers = GetCefKeyboardModifiers(wParam, lParam);

  browser_->GetHost()->SendKeyEvent(event);
}

void OsrWindowWin::OnPaint() {
  // Paint nothing here. Invalidate will cause OnPaint to be called for the
  // render handler.

    if (IsRectEmpty(&m_rcDirty)) return;

	PAINTSTRUCT ps;
	BeginPaint(hwnd_, &ps);

	DWORD dwExStyle = GetWindowLongPtr(hwnd_, GWL_EXSTYLE);
	if ((dwExStyle & WS_EX_LAYERED) != 0x80000)
		SetWindowLongPtr(hwnd_, GWL_EXSTYLE, dwExStyle ^ WS_EX_LAYERED);

    //内存DC跟着窗口走
    //网页离屏图像跟着回调走
    int nCurWidth = client_rect_.right - client_rect_.left;
    int nCurHeight = client_rect_.bottom - client_rect_.top;
    if (nCurWidth != m_nOldWidth || m_nOldHeight != nCurHeight)
    {
		if (m_hDcOffScreen)
		{
			if (m_hBmpOld) ::SelectObject(m_hDcOffScreen, m_hBmpOld);
			DeleteDC(m_hDcOffScreen);
			m_hDcOffScreen = NULL;
		}
		if (m_hBmpOffScreen)
		{
			DeleteObject(m_hBmpOffScreen);
			m_hBmpOffScreen = NULL;
		}
    }
	if (!m_hBmpOffScreen)
	{
		m_nOldWidth = nCurWidth;
		m_nOldHeight = nCurHeight;

		m_hDcOffScreen = ::CreateCompatibleDC(ps.hdc);
		BITMAPINFO bmi;
		memset(&bmi, 0, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = m_nOldWidth;
		bmi.bmiHeader.biHeight = -m_nOldHeight;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = 0;
		m_hBmpOffScreen = ::CreateDIBSection(ps.hdc, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
		m_hBmpOld = (HBITMAP)::SelectObject(m_hDcOffScreen, m_hBmpOffScreen);
	}

    
	::FillRect(m_hDcOffScreen, &m_rcDirty, m_hEraser);

	if (m_pOSRBitmap)
	{
		RECT rcWnd = { 0 };
		::GetWindowRect(hwnd_, &rcWnd);
		int nXDest = rcWnd.left;
		int nYDest = rcWnd.top;
	
		Gdiplus::Graphics gs(m_hDcOffScreen);
        gs.DrawImage(m_pOSRBitmap, 0, 0, m_pOSRBitmap->GetWidth(), m_pOSRBitmap->GetHeight());
        if (m_pPopBitmap) gs.DrawImage(m_pPopBitmap, m_rcPopUp.left, m_rcPopUp.top, m_pPopBitmap->GetWidth(), m_pPopBitmap->GetHeight());

		SIZE szWindow = { nCurWidth, nCurHeight };
		POINT ptSrc = { 0, 0 };
		POINT pt = { nXDest, nYDest };

		BLENDFUNCTION blendPixelFunction = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
   
		UPDATELAYEREDWINDOWINFO info = { 0 };
		info.cbSize = sizeof(UPDATELAYEREDWINDOWINFO);
		info.crKey = 0;
		info.dwFlags = ULW_ALPHA;
		info.hdcDst = NULL;
		info.hdcSrc = m_hDcOffScreen;
		info.pblend = &blendPixelFunction;
		info.pptDst = &pt;
		info.pptSrc = &ptSrc;
		info.psize = &szWindow;
		info.prcDirty = &m_rcDirty;
        UpdateLayeredWindowIndirect(hwnd_, &info);

		//::UpdateLayeredWindow(hwnd_, NULL, &pt, &szWindow, m_hDcOffScreen, &ptSrc, 0, &blendPixelFunction, ULW_ALPHA);
	}

	EndPaint(hwnd_, &ps);
}

bool OsrWindowWin::OnEraseBkgnd() {
  // Erase the background when the browser does not exist.
  return (browser_ == NULL);
}

bool OsrWindowWin::OnTouchEvent(UINT message, WPARAM wParam, LPARAM lParam) {
  // Handle touch events on Windows.
  int num_points = LOWORD(wParam);
  // Chromium only supports upto 16 touch points.
  if (num_points < 0 || num_points > 16)
    return false;
  std::unique_ptr<TOUCHINPUT[]> input(new TOUCHINPUT[num_points]);
  if (GetTouchInputInfo(reinterpret_cast<HTOUCHINPUT>(lParam), num_points,
                        input.get(), sizeof(TOUCHINPUT))) {
    CefTouchEvent touch_event;
    for (int i = 0; i < num_points; ++i) {
      POINT point;
      point.x = TOUCH_COORD_TO_PIXEL(input[i].x);
      point.y = TOUCH_COORD_TO_PIXEL(input[i].y);

      if (!IsWindows_8_Or_Newer()) {
        // Windows 7 sends touch events for touches in the non-client area,
        // whereas Windows 8 does not. In order to unify the behaviour, always
        // ignore touch events in the non-client area.
        LPARAM l_param_ht = MAKELPARAM(point.x, point.y);
        LRESULT hittest = SendMessage(hwnd_, WM_NCHITTEST, 0, l_param_ht);
        if (hittest != HTCLIENT)
          return false;
      }

      ScreenToClient(hwnd_, &point);
      touch_event.x = DeviceToLogical(point.x, device_scale_factor_);
      touch_event.y = DeviceToLogical(point.y, device_scale_factor_);

      // Touch point identifier stays consistent in a touch contact sequence
      touch_event.id = input[i].dwID;

      if (input[i].dwFlags & TOUCHEVENTF_DOWN) {
        touch_event.type = CEF_TET_PRESSED;
      } else if (input[i].dwFlags & TOUCHEVENTF_MOVE) {
        touch_event.type = CEF_TET_MOVED;
      } else if (input[i].dwFlags & TOUCHEVENTF_UP) {
        touch_event.type = CEF_TET_RELEASED;
      }

      touch_event.radius_x = 0;
      touch_event.radius_y = 0;
      touch_event.rotation_angle = 0;
      touch_event.pressure = 0;
      touch_event.modifiers = 0;

      // Notify the browser of touch event
      if (browser_)
        browser_->GetHost()->SendTouchEvent(touch_event);
    }
    CloseTouchInputHandle(reinterpret_cast<HTOUCHINPUT>(lParam));
    return true;
  }

  return false;
}

bool OsrWindowWin::IsOverPopupWidget(int x, int y) const {

    return false;
}

int OsrWindowWin::GetPopupXOffset() const {
	return 0;
}

int OsrWindowWin::GetPopupYOffset() const {
  return 0;
}

void OsrWindowWin::ApplyPopupOffset(int& x, int& y) const {
  if (IsOverPopupWidget(x, y)) {
    x += GetPopupXOffset();
    y += GetPopupYOffset();
  }
}

void OsrWindowWin::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  DCHECK(!browser_);
  browser_ = browser;
}

void OsrWindowWin::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  // Detach |this| from the ClientHandlerOsr.
  static_cast<ClientHandlerOsr*>(browser_->GetHost()->GetClient().get())
      ->DetachOsrDelegate();
  browser_ = NULL;
}

bool OsrWindowWin::GetRootScreenRect(CefRefPtr<CefBrowser> browser,
                                     CefRect& rect) {
  CEF_REQUIRE_UI_THREAD();
  return false;
}

void OsrWindowWin::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
  CEF_REQUIRE_UI_THREAD();
  DCHECK_GT(device_scale_factor_, 0);

  rect.x = rect.y = 0;
  rect.width = DeviceToLogical(client_rect_.right - client_rect_.left,
	  device_scale_factor_);
  if (rect.width == 0)
	  rect.width = 1;
  rect.height = DeviceToLogical(client_rect_.bottom - client_rect_.top,
	  device_scale_factor_);
  if (rect.height == 0)
	  rect.height = 1;

  //OutputDebugStringA("GetViewRect \n");
}

bool OsrWindowWin::GetScreenPoint(CefRefPtr<CefBrowser> browser,
                                  int viewX,
                                  int viewY,
                                  int& screenX,
                                  int& screenY) {
  CEF_REQUIRE_UI_THREAD();
  DCHECK_GT(device_scale_factor_, 0);

  if (!::IsWindow(hwnd_))
    return false;

  // Convert the point from view coordinates to actual screen coordinates.
  POINT screen_pt = {LogicalToDevice(viewX, device_scale_factor_),
                     LogicalToDevice(viewY, device_scale_factor_)};
  ClientToScreen(hwnd_, &screen_pt);
  screenX = screen_pt.x;
  screenY = screen_pt.y;
  return true;
}

bool OsrWindowWin::GetScreenInfo(CefRefPtr<CefBrowser> browser,
                                 CefScreenInfo& screen_info) {
  CEF_REQUIRE_UI_THREAD();
  DCHECK_GT(device_scale_factor_, 0);

  if (!::IsWindow(hwnd_))
    return false;

  CefRect view_rect;
  GetViewRect(browser, view_rect);

  screen_info.device_scale_factor = device_scale_factor_;

  // The screen info rectangles are used by the renderer to create and position
  // popups. Keep popups inside the view rectangle.
  screen_info.rect = view_rect;
  screen_info.available_rect = view_rect;
  return true;
}

void OsrWindowWin::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) {
    if (!show)
    {
        if (m_pPopBitmap)
        {
            delete m_pPopBitmap;
            m_pPopBitmap = NULL;
        }
    }
}

void OsrWindowWin::OnPopupSize(CefRefPtr<CefBrowser> browser,
                               const CefRect& rect) {
    m_rcPopUp.left = rect.x*GetDeviceScaleFactor();
    m_rcPopUp.top = rect.y* GetDeviceScaleFactor();
    m_rcPopUp.right = rect.width* GetDeviceScaleFactor() + rect.x;
    m_rcPopUp.bottom = rect.height* GetDeviceScaleFactor() + rect.y;
}

void OsrWindowWin::OnPaint(CefRefPtr<CefBrowser> browser,
                           CefRenderHandler::PaintElementType type,
                           const CefRenderHandler::RectList& dirtyRects,
                           const void* buffer,
                           int width,
                           int height) {

	if (type == PET_POPUP)
	{
		bool bRes = UpdateDirtyRect(&m_pPopBitmap, buffer, dirtyRects, width, height);
        if (bRes)
        {
			RECT rcTotalDirty = { 0 };
			for (UINT i = 0; i < dirtyRects.size(); i++)
			{
				RECT rcDirty = { dirtyRects[i].x,dirtyRects[i].y,dirtyRects[i].x + dirtyRects[i].width,dirtyRects[i].y + dirtyRects[i].height };
				UnionRect(&rcTotalDirty, &rcTotalDirty, &rcDirty);
			}
			
            OffsetRect(&rcTotalDirty, m_rcPopUp.left, m_rcPopUp.top);
			m_rcDirty = rcTotalDirty;
            ::InvalidateRect(hwnd_, &rcTotalDirty, false);
        }
	}
	else
	{
		bool bRes = UpdateDirtyRect(&m_pOSRBitmap, buffer, dirtyRects, width, height);
		if (bRes)
		{
			RECT rcTotalDirty = { 0 };
			for (UINT i = 0; i < dirtyRects.size(); i++)
			{
				RECT rcDirty = { dirtyRects[i].x,dirtyRects[i].y,dirtyRects[i].x + dirtyRects[i].width,dirtyRects[i].y + dirtyRects[i].height };
				UnionRect(&rcTotalDirty, &rcTotalDirty, &rcDirty);
			}
            m_rcDirty = rcTotalDirty;
			::InvalidateRect(hwnd_, &rcTotalDirty, false);
		}
	}
}

void OsrWindowWin::OnAcceleratedPaint(
    CefRefPtr<CefBrowser> browser,
    CefRenderHandler::PaintElementType type,
    const CefRenderHandler::RectList& dirtyRects,
    void* share_handle) {
 
}

void OsrWindowWin::OnCursorChange(CefRefPtr<CefBrowser> browser,
                                  CefCursorHandle cursor,
                                  CefRenderHandler::CursorType type,
                                  const CefCursorInfo& custom_cursor_info) {
  CEF_REQUIRE_UI_THREAD();

  if (!::IsWindow(hwnd_))
    return;

  // Change the plugin window's cursor.
  SetClassLongPtr(hwnd_, GCLP_HCURSOR,
                  static_cast<LONG>(reinterpret_cast<LONG_PTR>(cursor)));
  SetCursor(cursor);
}

bool OsrWindowWin::StartDragging(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDragData> drag_data,
    CefRenderHandler::DragOperationsMask allowed_ops,
    int x,
    int y) {
  CEF_REQUIRE_UI_THREAD();

#if defined(CEF_USE_ATL)
  if (!drop_target_)
    return false;

  current_drag_op_ = DRAG_OPERATION_NONE;
  CefBrowserHost::DragOperationsMask result =
      drop_target_->StartDragging(browser, drag_data, allowed_ops, x, y);
  current_drag_op_ = DRAG_OPERATION_NONE;
  POINT pt = {};
  GetCursorPos(&pt);
  ScreenToClient(hwnd_, &pt);

  browser->GetHost()->DragSourceEndedAt(
      DeviceToLogical(pt.x, device_scale_factor_),
      DeviceToLogical(pt.y, device_scale_factor_), result);
  browser->GetHost()->DragSourceSystemDragEnded();
  return true;
#else
  // Cancel the drag. The dragging implementation requires ATL support.
  return false;
#endif
}

void OsrWindowWin::UpdateDragCursor(CefRefPtr<CefBrowser> browser,
                                    CefRenderHandler::DragOperation operation) {
  CEF_REQUIRE_UI_THREAD();

#if defined(CEF_USE_ATL)
  current_drag_op_ = operation;
#endif
}

void OsrWindowWin::OnImeCompositionRangeChanged(
    CefRefPtr<CefBrowser> browser,
    const CefRange& selection_range,
    const CefRenderHandler::RectList& character_bounds) {
  CEF_REQUIRE_UI_THREAD();

  if (ime_handler_) {
    // Convert from view coordinates to device coordinates.
    CefRenderHandler::RectList device_bounds;
    CefRenderHandler::RectList::const_iterator it = character_bounds.begin();
    for (; it != character_bounds.end(); ++it) {
      device_bounds.push_back(LogicalToDevice(*it, device_scale_factor_));
    }

    ime_handler_->ChangeCompositionRange(selection_range, device_bounds);
  }
}

void OsrWindowWin::OnEditNodeFocused(int nX, int nY)
{
    m_nIEMX = nX;
    m_nIEMY = nY;
}

void OsrWindowWin::UpdateAccessibilityTree(CefRefPtr<CefValue> value) {
  CEF_REQUIRE_UI_THREAD();

#if defined(CEF_USE_ATL)
  if (!accessibility_handler_) {
    accessibility_handler_.reset(new OsrAccessibilityHelper(value, browser_));
  } else {
    accessibility_handler_->UpdateAccessibilityTree(value);
  }

  // Update |accessibility_root_| because UpdateAccessibilityTree may have
  // cleared it.
  OsrAXNode* root = accessibility_handler_->GetRootNode();
  accessibility_root_ = root ? root->GetNativeAccessibleObject(NULL) : NULL;
#endif  // defined(CEF_USE_ATL)
}

void OsrWindowWin::UpdateAccessibilityLocation(CefRefPtr<CefValue> value) {
  CEF_REQUIRE_UI_THREAD();

#if defined(CEF_USE_ATL)
  if (accessibility_handler_) {
    accessibility_handler_->UpdateAccessibilityLocation(value);
  }
#endif  // defined(CEF_USE_ATL)
}

bool OsrWindowWin::OnTooltip(CefRefPtr<CefBrowser> browser, CefString& text)
{
	if (m_szTooltip == text.ToWString()) {
		return false;
	}

    m_szTooltip = text.ToWString();
	HINSTANCE hInstance = ::GetModuleHandle(NULL);
	if (NULL == m_hTooltip) {
        m_hTooltip = ::CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS, NULL,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			hwnd_, NULL, hInstance, NULL);
	
		::ZeroMemory(&m_ToolInfo, sizeof(TOOLINFO));
        m_ToolInfo.cbSize = sizeof(TOOLINFO);
        m_ToolInfo.uFlags = TTF_IDISHWND;
        m_ToolInfo.hwnd = hwnd_;
        m_ToolInfo.uId = (UINT_PTR)hwnd_;
        m_ToolInfo.hinst = GetModuleHandle(NULL);
        m_ToolInfo.lpszText = const_cast<LPTSTR>((LPCTSTR)m_szTooltip.c_str());
 
		::SendMessage(m_hTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_ToolInfo));

		POINT pt;
		::GetCursorPos(&pt);
		pt.x += 5;
		pt.y += 5;

		::SendMessage(m_hTooltip, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));

		::SendMessage(m_hTooltip, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&m_ToolInfo));
	
	}
	else if (m_hTooltip != NULL && (m_szTooltip.empty() || !::IsWindowVisible(hwnd_) || ::IsIconic(hwnd_))) {

		::SendMessage(m_hTooltip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_ToolInfo));
	}
	else if (m_hTooltip != NULL && !m_szTooltip.empty()) {

		::SendMessage(m_hTooltip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_ToolInfo));
		::SendMessage(m_hTooltip, TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&m_ToolInfo));

        m_ToolInfo.lpszText = const_cast<LPWSTR>(m_szTooltip.c_str());
		::SendMessage(m_hTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_ToolInfo));
		::SendMessage(m_hTooltip, TTM_SETTOOLINFO, 0, reinterpret_cast<LPARAM>(&m_ToolInfo));
		
		POINT pt;
		::GetCursorPos(&pt);
		pt.x += 5;
		pt.y += 5;
		::SendMessage(m_hTooltip, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));
		::SendMessage(m_hTooltip, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&m_ToolInfo));
    }
	return false;
}

#if defined(CEF_USE_ATL)

CefBrowserHost::DragOperationsMask OsrWindowWin::OnDragEnter(
    CefRefPtr<CefDragData> drag_data,
    CefMouseEvent ev,
    CefBrowserHost::DragOperationsMask effect) {
  if (browser_) {
    DeviceToLogical(ev, device_scale_factor_);
    browser_->GetHost()->DragTargetDragEnter(drag_data, ev, effect);
    browser_->GetHost()->DragTargetDragOver(ev, effect);
  }
  return current_drag_op_;
}

CefBrowserHost::DragOperationsMask OsrWindowWin::OnDragOver(
    CefMouseEvent ev,
    CefBrowserHost::DragOperationsMask effect) {
  if (browser_) {
    DeviceToLogical(ev, device_scale_factor_);
    browser_->GetHost()->DragTargetDragOver(ev, effect);
  }
  return current_drag_op_;
}

void OsrWindowWin::OnDragLeave() {
  if (browser_)
    browser_->GetHost()->DragTargetDragLeave();
}

CefBrowserHost::DragOperationsMask OsrWindowWin::OnDrop(
    CefMouseEvent ev,
    CefBrowserHost::DragOperationsMask effect) {
  if (browser_) {
    DeviceToLogical(ev, device_scale_factor_);
    browser_->GetHost()->DragTargetDragOver(ev, effect);
    browser_->GetHost()->DragTargetDrop(ev);
  }
  return current_drag_op_;
}

#endif  // defined(CEF_USE_ATL)

bool OsrWindowWin::UpdateDirtyRect(Gdiplus::Bitmap** pDstBtmp,const void* buffer, const RectList& dirtyRects, int nWidth, int nHeight)
{
	bool bForceRepaint = false;
    if (!(*pDstBtmp))
    {
        (*pDstBtmp) = new Gdiplus::Bitmap(nWidth, nHeight);
        bForceRepaint = true;
    }
	else if ((*pDstBtmp)->GetWidth() != nWidth || (*pDstBtmp)->GetHeight() != nHeight)
	{
		delete (*pDstBtmp);
        (*pDstBtmp) = new Gdiplus::Bitmap(nWidth, nHeight);
		bForceRepaint = true;
	}
	if (!(*pDstBtmp)) return false;
    Gdiplus::BitmapData data;
    Gdiplus::Rect rcLock(0, 0, (*pDstBtmp)->GetWidth(), (*pDstBtmp)->GetHeight());
    (*pDstBtmp)->LockBits(&rcLock, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data);
	int nStride = (*pDstBtmp)->GetWidth() * 4;

	if (bForceRepaint)
	{
		for (int j = 0; j < rcLock.Height; j++)
		{
			unsigned char* src = (unsigned char*)buffer;
			unsigned char* dst = (unsigned char*)data.Scan0;
			src += j * nStride;
			dst += j * nStride;
			memcpy(dst, src, nStride);
		}
	}
	else
	{
		for (unsigned int i = 0; i < dirtyRects.size(); i++)
		{
			int nX = dirtyRects[i].x;
			if (nX > rcLock.Width) return true;

			int nY = dirtyRects[i].y;
			if (nY > rcLock.Height) return true;

			int nWidth = dirtyRects[i].width;
			if (nX + nWidth > rcLock.Width) return true;

			int nHeight = dirtyRects[i].height;
			if (nY + nHeight > rcLock.Height) return true;
			for (int j = 0; j < nHeight; j++)
			{
				unsigned char* src = (unsigned char*)buffer;
				unsigned char* dst = (unsigned char*)data.Scan0;
				src += (nY + j) * nStride + nX * 4;
				dst += (nY + j) * nStride + nX * 4;
				memcpy(dst, src, nWidth * 4);
			}
		}
	}
    (*pDstBtmp)->UnlockBits(&data);
    //OnQuickSave(pDstBtmp, L"D:\\test.png");
	return true;
}

}  // namespace client
