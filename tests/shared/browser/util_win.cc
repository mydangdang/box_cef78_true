// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "tests/shared/browser/util_win.h"

#include "include/base/cef_logging.h"
#include <shellscalingapi.h>
#include "tests/cefclient/FileAttribute.h"
#include <ShlObj.h>

namespace client {

namespace {

LARGE_INTEGER qi_freq_ = {};

}  // namespace

uint64_t GetTimeNow() {
  if (!qi_freq_.HighPart && !qi_freq_.LowPart) {
    QueryPerformanceFrequency(&qi_freq_);
  }
  LARGE_INTEGER t = {};
  QueryPerformanceCounter(&t);
  return static_cast<uint64_t>((t.QuadPart / double(qi_freq_.QuadPart)) *
                               1000000);
}

void SetUserDataPtr(HWND hWnd, void* ptr) {
  SetLastError(ERROR_SUCCESS);
  LONG_PTR result =
      ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ptr));
  CHECK(result != 0 || GetLastError() == ERROR_SUCCESS);
}

WNDPROC SetWndProcPtr(HWND hWnd, WNDPROC wndProc) {
  WNDPROC old =
      reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hWnd, GWLP_WNDPROC));
  CHECK(old != NULL);
  LONG_PTR result = ::SetWindowLongPtr(hWnd, GWLP_WNDPROC,
                                       reinterpret_cast<LONG_PTR>(wndProc));
  CHECK(result != 0 || GetLastError() == ERROR_SUCCESS);
  return old;
}

std::wstring GetResourceString(UINT id) {
#define MAX_LOADSTRING 100
  TCHAR buff[MAX_LOADSTRING] = {0};
  LoadString(::GetModuleHandle(NULL), id, buff, MAX_LOADSTRING);
  return buff;
}

int GetCefMouseModifiers(WPARAM wparam) {
  int modifiers = 0;
  if (wparam & MK_CONTROL)
    modifiers |= EVENTFLAG_CONTROL_DOWN;
  if (wparam & MK_SHIFT)
    modifiers |= EVENTFLAG_SHIFT_DOWN;
  if (IsKeyDown(VK_MENU))
    modifiers |= EVENTFLAG_ALT_DOWN;
  if (wparam & MK_LBUTTON)
    modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
  if (wparam & MK_MBUTTON)
    modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
  if (wparam & MK_RBUTTON)
    modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;

  // Low bit set from GetKeyState indicates "toggled".
  if (::GetKeyState(VK_NUMLOCK) & 1)
    modifiers |= EVENTFLAG_NUM_LOCK_ON;
  if (::GetKeyState(VK_CAPITAL) & 1)
    modifiers |= EVENTFLAG_CAPS_LOCK_ON;
  return modifiers;
}

int GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam) {
  int modifiers = 0;
  if (IsKeyDown(VK_SHIFT))
    modifiers |= EVENTFLAG_SHIFT_DOWN;
  if (IsKeyDown(VK_CONTROL))
    modifiers |= EVENTFLAG_CONTROL_DOWN;
  if (IsKeyDown(VK_MENU))
    modifiers |= EVENTFLAG_ALT_DOWN;

  // Low bit set from GetKeyState indicates "toggled".
  if (::GetKeyState(VK_NUMLOCK) & 1)
    modifiers |= EVENTFLAG_NUM_LOCK_ON;
  if (::GetKeyState(VK_CAPITAL) & 1)
    modifiers |= EVENTFLAG_CAPS_LOCK_ON;

  switch (wparam) {
    case VK_RETURN:
      if ((lparam >> 16) & KF_EXTENDED)
        modifiers |= EVENTFLAG_IS_KEY_PAD;
      break;
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
      if (!((lparam >> 16) & KF_EXTENDED))
        modifiers |= EVENTFLAG_IS_KEY_PAD;
      break;
    case VK_NUMLOCK:
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
    case VK_DIVIDE:
    case VK_MULTIPLY:
    case VK_SUBTRACT:
    case VK_ADD:
    case VK_DECIMAL:
    case VK_CLEAR:
      modifiers |= EVENTFLAG_IS_KEY_PAD;
      break;
    case VK_SHIFT:
      if (IsKeyDown(VK_LSHIFT))
        modifiers |= EVENTFLAG_IS_LEFT;
      else if (IsKeyDown(VK_RSHIFT))
        modifiers |= EVENTFLAG_IS_RIGHT;
      break;
    case VK_CONTROL:
      if (IsKeyDown(VK_LCONTROL))
        modifiers |= EVENTFLAG_IS_LEFT;
      else if (IsKeyDown(VK_RCONTROL))
        modifiers |= EVENTFLAG_IS_RIGHT;
      break;
    case VK_MENU:
      if (IsKeyDown(VK_LMENU))
        modifiers |= EVENTFLAG_IS_LEFT;
      else if (IsKeyDown(VK_RMENU))
        modifiers |= EVENTFLAG_IS_RIGHT;
      break;
    case VK_LWIN:
      modifiers |= EVENTFLAG_IS_LEFT;
      break;
    case VK_RWIN:
      modifiers |= EVENTFLAG_IS_RIGHT;
      break;
  }
  return modifiers;
}

bool IsKeyDown(WPARAM wparam) {
  return (GetKeyState(wparam) & 0x8000) != 0;
}

float GetDeviceScaleFactor() {
  static float scale_factor = 1.0;
  static bool initialized = false;

  if (!initialized) {
    // This value is safe to cache for the life time of the app since the user
    // must logout to change the DPI setting. This value also applies to all
    // screens.
    HDC screen_dc = ::GetDC(NULL);
    int dpi_x = GetDeviceCaps(screen_dc, LOGPIXELSX);
    scale_factor = static_cast<float>(dpi_x) / 96.0f;
    ::ReleaseDC(NULL, screen_dc);
    initialized = true;
  }

  return scale_factor;
}

bool IsProcessPerMonitorDpiAware()
{
	enum class PerMonitorDpiAware {
		UNKNOWN = 0,
		PER_MONITOR_DPI_UNAWARE,
		PER_MONITOR_DPI_AWARE,
	};
	static PerMonitorDpiAware per_monitor_dpi_aware = PerMonitorDpiAware::UNKNOWN;
	if (per_monitor_dpi_aware == PerMonitorDpiAware::UNKNOWN) {
		per_monitor_dpi_aware = PerMonitorDpiAware::PER_MONITOR_DPI_UNAWARE;
		HMODULE shcore_dll = ::LoadLibrary(L"shcore.dll");
		if (shcore_dll) {
			typedef HRESULT(WINAPI* GetProcessDpiAwarenessPtr)(
				HANDLE, PROCESS_DPI_AWARENESS*);
			GetProcessDpiAwarenessPtr func_ptr =
				reinterpret_cast<GetProcessDpiAwarenessPtr>(
					::GetProcAddress(shcore_dll, "GetProcessDpiAwareness"));
			if (func_ptr) {
				PROCESS_DPI_AWARENESS awareness;
				if (SUCCEEDED(func_ptr(nullptr, &awareness)) &&
					awareness == PROCESS_PER_MONITOR_DPI_AWARE)
					per_monitor_dpi_aware = PerMonitorDpiAware::PER_MONITOR_DPI_AWARE;
			}
		}
	}
	return per_monitor_dpi_aware == PerMonitorDpiAware::PER_MONITOR_DPI_AWARE;
}

float GetWindowScaleFactor(HWND hwnd)
{
	if (hwnd && IsProcessPerMonitorDpiAware()) {
		typedef UINT(WINAPI* GetDpiForWindowPtr)(HWND);
		static GetDpiForWindowPtr func_ptr = reinterpret_cast<GetDpiForWindowPtr>(
			GetProcAddress(GetModuleHandle(L"user32.dll"), "GetDpiForWindow"));
		if (func_ptr)
			return static_cast<float>(func_ptr(hwnd)) / DPI_1X;
	}

	return client::GetDeviceScaleFactor();
}

std::string g_szPath;

std::string DataPath()
{
	if (g_szPath.empty())
	{
		char szModulePath[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, szModulePath, MAX_PATH);
		std::string szExePath = szModulePath;
		std::string szProductName;
		UtilFileAttribute::GetProductName(szExePath, szProductName);
		char szAppData[MAX_PATH + 1] = { 0 };
		SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, szAppData);
		g_szPath = szAppData;
		g_szPath = g_szPath + "\\" + szProductName;
	}
	return g_szPath;
}

}  // namespace client
