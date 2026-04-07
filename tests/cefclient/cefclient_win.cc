// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
#include <winsock2.h>
#include <windows.h>
#include <Shlwapi.h>
#include "include/base/cef_scoped_ptr.h"
#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"
#include "tests/cefclient/browser/main_context_impl.h"
#include "tests/cefclient/browser/main_message_loop_multithreaded_win.h"
#include "tests/cefclient/browser/root_window_manager.h"
#include "tests/cefclient/browser/test_runner.h"
#include "tests/shared/browser/client_app_browser.h"
#include "tests/shared/browser/main_message_loop_external_pump.h"
#include "tests/shared/browser/main_message_loop_std.h"
#include "tests/shared/common/client_app_other.h"
#include "tests/shared/common/client_switches.h"
#include "tests/shared/renderer/client_app_renderer.h"
#include <process.h>
#include "RTCTX.h"
#include "string_util.h"
#include "tests/cefclient/browser/resource.h"
#include "MiniDumper.h"
#include "SysTask.h"
#include "libMinHook/MinHook.h"
#include <corecrt_io.h>
#include <ShlObj.h>
#include "../shared/browser/util_win.h"
#include "InitialWnd.h"
#include "MemoryDeal.h"
#include <winternl.h>

//#include "tests/cefclient/browser/TopFullToolWnd.h"

// When generating projects with CMake the CEF_USE_SANDBOX value will be defined
// automatically if using the required compiler version. Pass -DUSE_SANDBOX=OFF
// to the CMake command-line to disable use of the sandbox.
// Uncomment this line to manually enable sandbox support.
// #define CEF_USE_SANDBOX 1

#if defined(CEF_USE_SANDBOX)
// The cef_sandbox.lib static library may not link successfully with all VS
// versions.
#pragma comment(lib, "cef_sandbox.lib")
#endif

typedef BOOL(WINAPI* realCreateProcessWPtr)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
	);


typedef BOOL(WINAPI* realCreateProcessAPtr)(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
	LPCSTR lpCurrentDirectory, \
	LPSTARTUPINFOA lpStartupInfo, \
	LPPROCESS_INFORMATION lpProcessInformation);

realCreateProcessWPtr fRealCreateProcessW = NULL;
realCreateProcessAPtr fRealCreateProcessA = NULL;

BOOL WINAPI MyCreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
	LPCSTR lpCurrentDirectory, \
	LPSTARTUPINFOA lpStartupInfo, \
	LPPROCESS_INFORMATION lpProcessInformation) {

	if (lpCommandLine != NULL) {
		std::string strCommandLine = lpCommandLine;
		if (std::string::npos != strCommandLine.find("echo NOT SANDBOXED"))
		{
			return TRUE;
		}
		else
		{
			return fRealCreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		}
	}
	else
	{
		return fRealCreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}
}

BOOL WINAPI MyCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
) {

	if (lpCommandLine != NULL) {
		std::wstring strCommandLine(lpCommandLine);
		if (std::string::npos != strCommandLine.find(L"echo NOT SANDBOXED"))
		{
			return TRUE;
		}
		else
		{
			return fRealCreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		}
	}
	else {
		return fRealCreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}
}

typedef INT(WSAAPI* realGetAddrInfoPtr)(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA* pHints, PADDRINFOA* ppResult);
realGetAddrInfoPtr fRealGetAddrInfo = NULL;
INT WSAAPI MyGetAddrInfo(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA* pHints, PADDRINFOA* ppResult)
{
	std::string szHostName = pNodeName;
	if (szHostName.find("macromedia.com") != -1
		|| szHostName.find("adobe.com") != -1)
	{
		OutputDebugStringA("HIT Host");
		return WSAHOST_NOT_FOUND;
	}
	return fRealGetAddrInfo(pNodeName, pServiceName, pHints, ppResult);
}

typedef HANDLE
(WINAPI* 
realCreateFileW)(
	_In_ LPCWSTR lpFileName,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ HANDLE hTemplateFile
);
realCreateFileW fRealCreateFileW = NULL;
HANDLE WINAPI MyCreateFileW(
	_In_ LPCWSTR lpFileName,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ HANDLE hTemplateFile
)
{
	std::wstring szFileName = lpFileName;
	if (szFileName.find(L"settings.sol") != -1)
	{
		OutputDebugStringW(L"HIT CreateW");

		TCHAR cName[MAX_PATH] = { 0 };
		GetModuleFileName(NULL, cName, MAX_PATH);
		PathRemoveFileSpec(cName);
		std::wstring szPath = cName;
		szPath += L"\\pftmp";
		
		return fRealCreateFileW(szPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	return fRealCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

typedef BOOL
(WINAPI*realReadFile)(
	_In_ HANDLE hFile,
	_Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer,
	_In_ DWORD nNumberOfBytesToRead,
	_Out_opt_ LPDWORD lpNumberOfBytesRead,
	_Inout_opt_ LPOVERLAPPED lpOverlapped
);
realReadFile fRealReadFile = NULL;
BOOL WINAPI MyReadFile(
	_In_ HANDLE hFile,
	_Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer,
	_In_ DWORD nNumberOfBytesToRead,
	_Out_opt_ LPDWORD lpNumberOfBytesRead,
	_Inout_opt_ LPOVERLAPPED lpOverlapped
)
{
	TCHAR Path[MAX_PATH] = { 0 };
	GetFinalPathNameByHandle(hFile, Path, MAX_PATH, VOLUME_NAME_NT);
	std::wstring szPath = Path;
	if (szPath.find(L"settings.sol") != -1)
	{
		OutputDebugStringW(L"HIT Read");
		return 0;
	}
	return fRealReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
	
}


typedef NTSTATUS(WINAPI* fLdrLoadDll) (PWSTR SearchPath OPTIONAL, PULONG DllCharacteristics OPTIONAL, PUNICODE_STRING DllName, PVOID* BaseAddress);
fLdrLoadDll fRealLdrLoadDll = NULL;
NTSTATUS WINAPI MyLdrLoadDll(PWSTR SearchPath OPTIONAL, PULONG DllCharacteristics OPTIONAL, PUNICODE_STRING DllName, PVOID* BaseAddress)
{
	NTSTATUS hRes = fRealLdrLoadDll(SearchPath, DllCharacteristics, DllName, BaseAddress);
	std::wstring szDllName(DllName->Buffer, DllName->Length);
	if (szDllName.find(L"pepflashplayer.dll") != -1)
	{
		if (*(char**)BaseAddress == 0) return hRes;
		char* pBase = *(char**)BaseAddress + 0xCC20B8;
		DWORD dwOldProtect;
		VirtualProtect(pBase, 8, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		char szMagic[8] = { 0x00,0x00,0x40,0x46,0x3e,0x6f,0x77,0x42 };
		if (!memcmp(pBase, szMagic, 8))
		{
			char sTime[8] = { 0 };
			double* pNow = new double;
			*pNow = (time(NULL) + 3600 * 24 * 3) * 1000;
			memcpy_s(sTime, 8, pNow, 8);
			memcpy_s(pBase, 8, sTime, 8);
		}
		VirtualProtect(pBase, 8, dwOldProtect, &dwOldProtect);
	}
	return hRes;
}

class HookFlashEchoWnd
{
public:
	HookFlashEchoWnd()
	{
		MH_Initialize();


// 		MH_CreateHook(&CreateProcessA, &MyCreateProcessA, reinterpret_cast<LPVOID*>(&fRealCreateProcessA));
// 		MH_CreateHook(&CreateProcessW, &MyCreateProcessW, reinterpret_cast<LPVOID*>(&fRealCreateProcessW));
 		MH_CreateHook(&CreateFileW, &MyCreateFileW, reinterpret_cast<LPVOID*>(&fRealCreateFileW));
		//MH_CreateHook(&ReadFile, &MyReadFile, reinterpret_cast<LPVOID*>(&fRealReadFile));
		MH_CreateHookApi(L"ws2_32.dll", "getaddrinfo", &MyGetAddrInfo, reinterpret_cast<LPVOID*>(&fRealGetAddrInfo));
//   		LPVOID fpLdrLoadDll = (LPVOID)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "LdrLoadDll");
//   		if (fpLdrLoadDll) MH_CreateHook(fpLdrLoadDll, &MyLdrLoadDll, reinterpret_cast<LPVOID*>(&fRealLdrLoadDll));
// 		
		MH_EnableHook(MH_ALL_HOOKS);
	}
	~HookFlashEchoWnd()
	{
		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
	}
};

namespace client {
namespace {

	HANDLE g_hSingleTonSem = NULL;
	int OnProgramEntry(HINSTANCE hInstance, LPCTSTR lpProcessName, std::string szClockName, bool bAllowMultiInstance)
	{
		DWORD dwCnt = 0;
		TCHAR szShareData[64] = { 0 };
		LoadString(hInstance, 101, szShareData, 64);
		std::wstring strSingleTonTag = lpProcessName;
		strSingleTonTag += szShareData;
		strSingleTonTag += +L"MainFrame";
		g_hSingleTonSem = CreateSemaphore(NULL, 1, 1, strSingleTonTag.c_str());
		if (ERROR_ALREADY_EXISTS == GetLastError())
		{
			HWND hwnd = NULL;
			
			DWORD dwPID = 0;
			DWORD dwStartTime = 0;
			HANDLE hShareDataMap = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, szShareData);
			if (hShareDataMap)
			{
				char *pData = (char *)::MapViewOfFile(hShareDataMap, FILE_MAP_READ, 0, 0, 0);
				memcpy(&dwCnt, pData, 4);
				memcpy(&dwPID, pData + 4 + (dwCnt-1) * 12, 4);
				memcpy(&dwStartTime,( pData + 4 + (dwCnt - 1) * 12) + 4, 4);
				memcpy(&hwnd, (pData + 4 + (dwCnt - 1) * 12) + 8, 4);
				UnmapViewOfFile(pData);
				CloseHandle(hShareDataMap);
			}
			CloseHandle(g_hSingleTonSem);
			if (bAllowMultiInstance) return dwCnt;

			if (hwnd)
			{
				if(IsWindow(hwnd))
				{
					COPYDATASTRUCT data;
					data.dwData = 0;
					data.cbData = szClockName.size();
					data.lpData = (LPVOID)szClockName.c_str();
					SendMessage(hwnd, WM_COPYDATA, (WPARAM)GetCurrentProcessId(), (LPARAM)&data);
					return dwCnt;
				}
			}
			else if (dwStartTime != 0 && dwStartTime + 3 >= GetTickCount()) return dwCnt;

			if (!dwPID)
			{
				HANDLE ProcessHandle = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, dwPID);
				if (ProcessHandle != NULL)
				{
					if(TerminateProcess(ProcessHandle, 0)) WaitForSingleObject(ProcessHandle, 5000);
					CloseHandle(ProcessHandle);
				}
			}
		}
		return dwCnt;
	}

	LRESULT CALLBACK MessageWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		MainContextImpl* pCtx = (MainContextImpl*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		switch (message)
		{
		case WM_NCCREATE:
		{
			CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
			MainContextImpl* pThis = reinterpret_cast<MainContextImpl*>(cs->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		}
		break;
		case WM_NCDESTROY:
		{
			SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
		}
		break;
		case WM_COPYDATA:
		{
			pCtx->GetRootWindowManager()->ReleaseBossKey();

			CefRefPtr<CefBrowser> browser = pCtx->GetRootWindowManager()->GetActiveBrowser();
			if (browser) {
				HWND hHostWnd = browser->GetHost()->GetWindowHandle();
				HWND hRootWnd = GetParent(hHostWnd);
				
				if (IsWindowVisible(hRootWnd)) {
					PostMessage(hRootWnd, WM_SHOWWINDOW, 0, 0);
				}
				if (IsIconic(hRootWnd)) {
					PostMessage(hRootWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
				}
				SetForegroundWindow(hRootWnd);
			}

			PCOPYDATASTRUCT pData = (PCOPYDATASTRUCT)lParam;
			if (pData->cbData)
			{
				std::string szClock = std::string((char*)pData->lpData, pData->cbData);
				MainContextImpl::Get()->OnClock(szClock);
			}
		}
		break;
		case WM_USER+1001:
		{
			//tray icon
			UINT uTrayMsg = (UINT)lParam;
			if (uTrayMsg == WM_LBUTTONDBLCLK)
			{
				CefRefPtr<CefBrowser> browser = pCtx->GetRootWindowManager()->GetActiveBrowser();
				if (browser) {
					HWND hHostWnd = browser->GetHost()->GetWindowHandle();
					HWND hRootWnd = GetParent(hHostWnd);

					if (IsWindowVisible(hRootWnd)) {
						PostMessage(hRootWnd, WM_SHOWWINDOW, 0, 0);
					}
					if (IsIconic(hRootWnd)) {
						PostMessage(hRootWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
					}
					SetForegroundWindow(hRootWnd);
				}
			}
			else if (uTrayMsg == WM_RBUTTONDOWN)
			{
				POINT point;
				::GetCursorPos(&point);
				HMENU hMenuPopup = CreatePopupMenu();
				AppendMenu(hMenuPopup, MF_STRING, 1, L"显示主程序");
				AppendMenu(hMenuPopup, MF_STRING, 2, L"退出程序");
				TrackPopupMenu(hMenuPopup, TPM_RIGHTBUTTON, point.x, point.y, 0, hWnd, NULL);
				::PostMessage(hWnd, WM_NULL, 0, 0);
				DestroyMenu(hMenuPopup);
			}
		}
		break;
		case WM_COMMAND:
		{
			switch (wParam)
			{
			case 1:
			{
				CefRefPtr<CefBrowser> browser = pCtx->GetRootWindowManager()->GetActiveBrowser();
				if (browser) {
					HWND hHostWnd = browser->GetHost()->GetWindowHandle();
					HWND hRootWnd = GetParent(hHostWnd);

					if (IsWindowVisible(hRootWnd)) {
						PostMessage(hRootWnd, WM_SHOWWINDOW, 0, 0);
					}
					if (IsIconic(hRootWnd)) {
						PostMessage(hRootWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
					}
					SetForegroundWindow(hRootWnd);
				}
			}
				break;
			case 2:
			{
				CefRefPtr<CefBrowser> browser = pCtx->GetRootWindowManager()->GetActiveBrowser();
				HWND hHostWnd = GetParent(browser->GetHost()->GetWindowHandle());
				PostMessage(hHostWnd, WM_CLOSE, browser->GetIdentifier(), 0);
			}
				break;
			default:
				break;
			}
		}
			break;
		default:
			break;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}



int RunMain(HINSTANCE hInstance, int nCmdShow) {
  // Enable High-DPI support on Windows 7 or newer.
  CefEnableHighDPISupport();

  CefMainArgs main_args(hInstance);

  void* sandbox_info = NULL;
#if defined(CEF_USE_SANDBOX)
  // Manage the life span of the sandbox information object. This is necessary
  // for sandbox support on Windows. See cef_sandbox_win.h for complete details.
  CefScopedSandboxInfo scoped_sandbox;
  sandbox_info = scoped_sandbox.sandbox_info();
#endif

  // Parse command-line arguments.
  CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
  command_line->InitFromString(::GetCommandLineW());

  HookFlashEchoWnd HookEcho;
  // Create a ClientApp of the correct type.
  CefRefPtr<CefApp> app;
  ClientApp::ProcessType process_type = ClientApp::GetProcessType(command_line);

  MemoryDealer.Init(process_type == ClientApp::BrowserProcess ? true:false);

  if (process_type == ClientApp::BrowserProcess)
    app = new ClientAppBrowser();
  else if (process_type == ClientApp::RendererProcess)
    app = new ClientAppRenderer();
  else if (process_type == ClientApp::OtherProcess)
  {
	  std::string szCmdLine = ::GetCommandLineA();
	  if (szCmdLine.find("ppapi") != -1)
	  {
		  //system("pause");
	  }
	  app = new ClientAppOther();
  }

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app, sandbox_info);
  if (exit_code >= 0)
    return exit_code;

  WCHAR app_path[MAX_PATH] = { 0 };
  GetModuleFileName(NULL, app_path, MAX_PATH);
  PathRemoveFileSpec(app_path);

  std::wstring cmd_path = app_path;
  cmd_path += L"\\sandbox.exe";
  SetEnvironmentVariable(L"ComSpec", cmd_path.c_str());

  std::string szClock;
  if (command_line->HasSwitch("clock"))
	  szClock = command_line->GetSwitchValue("clock");

  bool bAllowMulti = false;
  TCHAR szShareData[64] = { 0 };
  LoadString(hInstance, 114, szShareData, 64);
  std::wstring wszAllowMulti = szShareData;
  if (wszAllowMulti == L"1") bAllowMulti = true;
  else bAllowMulti = false;

  int nExistInstanceCnt = OnProgramEntry(hInstance,UtilString::SA2W(RTCTX::ExeName()).c_str(), szClock, bAllowMulti);
  if ((!bAllowMulti && nExistInstanceCnt > 0) || nExistInstanceCnt >= 60) return nExistInstanceCnt;

  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  if (FAILED(hr))
  {
	  CoUninitialize();
  }
  hr = CoInitializeSecurity(NULL,-1,NULL,NULL,RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE,NULL,0, NULL);
  if (FAILED(hr))
  {
	  CoUninitialize();
  }

  static CMiniDumper stMindumper(true);

  memset(szShareData, 0, 64);
  LoadString(hInstance, 101, szShareData, 64);


  HANDLE hShareData = NULL;
  char* pShareData = NULL;

  if(nExistInstanceCnt > 0)
  {
	  hShareData = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, szShareData);
	  if (hShareData) pShareData = (char*)::MapViewOfFile(hShareData, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  }
  else
  {
	  hShareData = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4+600, szShareData);
	  if (hShareData)  pShareData = (char*)::MapViewOfFile(hShareData, FILE_MAP_ALL_ACCESS, 0, 0, 0); 
  }

  if (pShareData)
  {
	  nExistInstanceCnt += 1;
	  memcpy(pShareData, &nExistInstanceCnt, 4);

	  int nPID = GetCurrentProcessId();
	  memcpy(pShareData + 4 + (nExistInstanceCnt - 1)* 12, &nPID, 4);

	  DWORD dwStartTime = GetTickCount();
	  memcpy(pShareData + 4 + (nExistInstanceCnt - 1) * 12 + 4, &dwStartTime, 4);
  }

  InitialWnd initailWnd;
  initailWnd.CreateInitalWnd();
  initailWnd.SetVisible(true);
  initailWnd.DoModal();

  // Create the main context object.
  scoped_ptr<MainContextImpl> context(new MainContextImpl(command_line, true,InitialWnd::m_szCfg));

  context->m_szShareData = UtilString::SW2A(szShareData);
  if(pShareData)
  {
	  WNDCLASSEX wc = { 0 };
	  wc.cbSize = sizeof(wc);
	  wc.lpfnWndProc = MessageWndProc;
	  wc.hInstance = hInstance;
	  wc.lpszClassName = L"Box_MsgWnd";
	  ::RegisterClassEx(&wc);

	  HWND hMsgWnd = CreateWindowEx(0, L"Box_MsgWnd", 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, hInstance, context->Get());
	  memcpy_s(pShareData + 4 + (nExistInstanceCnt - 1) * 12 + 8, 4, &hMsgWnd, 4);
	  context->CreateTrayICON(hInstance, hMsgWnd);
  }

  CefSettings settings;

#if !defined(CEF_USE_SANDBOX)
  settings.no_sandbox = true;
#endif

  // Applications should specify a unique GUID here to enable trusted downloads.
  CefString(&settings.application_client_id_for_file_scanning)
      .FromString("9A8DE24D-B822-4C6C-8259-5A848FEA1E68");

  // Populate the settings based on command line arguments.
  context->PopulateSettings(&settings);

  // Create the main message loop object.
  scoped_ptr<MainMessageLoop> message_loop;
  if (settings.multi_threaded_message_loop)
    message_loop.reset(new MainMessageLoopMultithreadedWin);
  else if (settings.external_message_pump)
    message_loop = MainMessageLoopExternalPump::Create();
  else
    message_loop.reset(new MainMessageLoopStd);


  std::string strPath = client::DataPath();
  strPath += "\\cache";
  std::string mainCache = strPath + "\\main";
  
  CefString(&settings.cache_path).FromWString(UtilString::SA2W(mainCache).c_str());
  CefString(&settings.root_cache_path).FromWString(UtilString::SA2W(strPath).c_str());
  CefString(&settings.user_data_path).FromWString(UtilString::SA2W(strPath).c_str());
  CefString(&settings.locale).FromWString(L"zh-CN");
  CefString(&settings.accept_language_list).FromWString(L"zh-CN,zh");
#ifndef _DEBUG
  settings.log_severity = LOGSEVERITY_DISABLE;
#endif
  settings.persist_session_cookies = 1;
  settings.persist_user_preferences = 1;

  // Initialize CEF.
  context->Initialize(main_args, settings, app, sandbox_info);

  // Register scheme handlers.
  test_runner::RegisterSchemeHandlers();

  RootWindowConfig window_config;
  window_config.always_on_top = command_line->HasSwitch(switches::kAlwaysOnTop);
  
  //window_config.with_controls = !command_line->HasSwitch(switches::kHideControls);
  window_config.with_controls = false;
  window_config.with_osr = settings.windowless_rendering_enabled ? true : false;

  RECT rcPos = MainContextImpl::Get()->GetInitPos();
  window_config.bounds.x = rcPos.left;
  window_config.bounds.y = rcPos.top;
  window_config.bounds.width = rcPos.right- rcPos.left;
  window_config.bounds.height = rcPos.bottom-rcPos.top;
  window_config.initially_hidden = true;
  // Create the first window.
  context->GetRootWindowManager()->CreateRootWindow(window_config);

  // Run the message loop. This will block until Quit() is called by the
  // RootWindowManager after all windows have been destroyed.
  int result = message_loop->Run();

  // Shut down CEF.
  context->Shutdown();

  if (pShareData) UnmapViewOfFile(pShareData);
  if (hShareData) CloseHandle(hShareData);
  if (g_hSingleTonSem)
  {
	  ReleaseSemaphore(g_hSingleTonSem, 1, NULL);
	  CloseHandle(g_hSingleTonSem);
  }
  CoUninitialize();

  // Release objects in reverse order of creation.
  message_loop.reset();
  context.reset();

  return result;
}

}  // namespace
}  // namespace client

// Program entry point function.
int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine,
                      int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  DWORD dwMode = GetErrorMode();
  dwMode = dwMode | SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT | SEM_NOOPENFILEERRORBOX;
  SetErrorMode(dwMode);

  return client::RunMain(hInstance, nCmdShow);
}
