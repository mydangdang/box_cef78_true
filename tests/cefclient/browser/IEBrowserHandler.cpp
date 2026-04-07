#include "IEBrowserHandler.h"
#include "include/cef_values.h"
#include "tests/cefclient/RTCTX.h"
#include "tests/cefclient/string_util.h"
#include "include/cef_parser.h"
#include <tests\shared\browser\util_win.h>
#include <wininet.h>

#include "tests/cefclient/browser/main_context_impl.h"
#include "tests/cefclient/browser/root_window_win.h"
#include "tests/cefclient/WinMsg.h"

#include "tests/cefclient/MemoryDeal.h"
//#include "../Injecter.h"

#pragma comment(lib, "wininet")
const wchar_t* g_IEHostClassName = L"IEKernelBrowser";

IEBrowserHandler::IEBrowserHandler(IEDelegate* pDelegate,std::string szURL)
{
	m_bMute = false;
	m_dwIEPid = 0;
	m_dwIEExtraPid = 0;
	m_szURL = szURL;
	m_hIEWnd = NULL;
	m_hProcess = NULL;
	m_hHostWnd = NULL;
	m_hExitEvent = NULL;
	m_pAudioMgr = NULL;
	m_hShareData = NULL;
	m_pShareData = NULL;
	m_pDelegate = pDelegate;
	m_hInstance = GetModuleHandle(NULL);
	RegMainClass();
}

IEBrowserHandler::~IEBrowserHandler()
{
	if (m_pShareData) UnmapViewOfFile(m_pShareData);
	if (m_hShareData) CloseHandle(m_hShareData);
}

LRESULT WINAPI IEHostWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
// 	char szOut[1024] = { 0 };
// 	sprintf_s(szOut, "IE:%20s %x %x %x\n", GetMessageText(uMsg), hWnd, wParam, lParam);
// 	OutputDebugStringA(szOut);
	IEBrowserHandler* pIEHandler = (IEBrowserHandler*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_USER+1001:
		pIEHandler->OnBrowserCreated((HWND)wParam);
		break;
	case WM_USER+119:
	{
		if(pIEHandler)
		{
			pIEHandler->m_dwIEExtraPid = (DWORD)wParam;
			pIEHandler->OnExtraProcessCreate();
		}
	}
	break;
	case WM_NCCREATE:
	{
		CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		IEBrowserHandler* pThis = reinterpret_cast<IEBrowserHandler*>(cs->lpCreateParams);
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
		PCOPYDATASTRUCT pData = (PCOPYDATASTRUCT)lParam;
		if (pData->cbData && pIEHandler)
		{
			switch (pData->dwData)
			{
			case 0:
			{
				int nTarget = (int)wParam;
				std::string szMsg = std::string((char*)pData->lpData, pData->cbData);
				pIEHandler->BroadCast(szMsg, nTarget);
			}
				break;
			case 1:
			{
				std::string szURL = std::string((char*)pData->lpData, pData->cbData);
				pIEHandler->BeforePopup(szURL);
			}
				break;
			case 2:
				pIEHandler->OnTitleChange(std::string((char*)pData->lpData, pData->cbData));
			break;
			case 3:
				pIEHandler->OnFlashError(std::string((char*)pData->lpData, pData->cbData));
			break;
			case 5:
				pIEHandler->OnExtraProcessPay(std::string((char*)pData->lpData, pData->cbData));
			break;
			default:
				break;
			}
		}
	}
		break;

	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void IEBrowserHandler::RegMainClass()
{
	WNDCLASS wc;
	wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;
	wc.hInstance = m_hInstance;
	wc.lpfnWndProc = IEHostWndProc;
	wc.lpszClassName = g_IEHostClassName;
	wc.lpszMenuName = NULL;
	wc.style = 0;
	RegisterClass(&wc);
}

bool IEBrowserHandler::CreateBrowser(HWND hParent, const CefRect& rcMargin)
{

	HWND hAncestor = GetAncestor(hParent, GA_ROOT);
	RECT rcAncestor = { 0 };
	GetWindowRect(hAncestor, &rcAncestor);
	int nX = rcMargin.x + rcAncestor.left;
	int nY = rcMargin.y + rcAncestor.top;


	m_hHostWnd = CreateWindowEx(WS_EX_TOOLWINDOW, g_IEHostClassName, g_IEHostClassName,
		WS_CHILDWINDOW | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE,
		nX, nY, rcMargin.width, rcMargin.height, hParent, NULL, m_hInstance, this);
	

	CefRefPtr<CefDictionaryValue> processCMD = CefDictionaryValue::Create();
	processCMD->SetInt("parent", (int)m_hHostWnd);
	processCMD->SetInt("pid", GetCurrentProcessId());
	processCMD->SetString("url", m_szURL.c_str());
	if(m_nFixFlash > 0) processCMD->SetInt("fixflash", m_nFixFlash);
	
	CefRefPtr<CefValue> value = CefValue::Create();
	value->SetDictionary(processCMD);
	CefString scefCmd = CefWriteJSON(value, JSON_WRITER_DEFAULT);

	std::string szCmd = scefCmd.ToString();
	szCmd = UtilString::UrlEncode(szCmd);
	std::string szIEPath = RTCTX::CurPath() + "\\eweb.exe";


	PROCESS_INFORMATION pi;
	LPSECURITY_ATTRIBUTES lpAtt = new SECURITY_ATTRIBUTES;
	lpAtt->bInheritHandle = TRUE;
	lpAtt->lpSecurityDescriptor = NULL;
	lpAtt->nLength = sizeof(SECURITY_ATTRIBUTES);
	STARTUPINFOA si = { sizeof(si) };
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = TRUE;

	if (m_hExitEvent)
	{
		SetEvent(m_hExitEvent);
		CloseHandle(m_hExitEvent);
	}

	m_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	BOOL bRet = ::CreateProcessA(szIEPath.c_str(), (LPSTR)szCmd.c_str(),
		lpAtt, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	m_hProcess = pi.hProcess;
	m_dwIEPid = pi.dwProcessId;
	if (bRet)
	{
		std::thread tDeamon(&IEBrowserHandler::Deamon, this, szIEPath, szCmd, m_hProcess, m_hExitEvent);
		tDeamon.detach();
		MemoryDealer.MonitePID(m_dwIEPid, m_hProcess);
	}
	::CloseHandle(pi.hThread);
	if (lpAtt)
	{
		delete lpAtt;
		lpAtt = NULL;
	}

	return bRet;

}


void IEBrowserHandler::Deamon(std::string szPath, std::string szCmd, HANDLE hProcess, HANDLE hExitEvent)
{
	HANDLE hEvents[2] = {hProcess,hExitEvent};

	while (1)
	{
		DWORD dwRes = WaitForMultipleObjects(2, hEvents, false, -1);
		if (dwRes == WAIT_OBJECT_0)
		{
			PROCESS_INFORMATION pi;
			LPSECURITY_ATTRIBUTES lpAtt = new SECURITY_ATTRIBUTES;
			lpAtt->bInheritHandle = TRUE;
			lpAtt->lpSecurityDescriptor = NULL;
			lpAtt->nLength = sizeof(SECURITY_ATTRIBUTES);
			STARTUPINFOA si = {sizeof(si)};
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = TRUE;

			BOOL bRet = ::CreateProcessA(szPath.c_str(), (LPSTR)szCmd.c_str(),lpAtt, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
			if (!bRet) break;

			m_hProcess = pi.hProcess;
			m_dwIEPid = pi.dwProcessId;
			m_pDelegate->OnIEProcessDeamon((int)m_hProcess);
			hEvents[0] = m_hProcess;
			MemoryDealer.MonitePID(m_dwIEPid, m_hProcess);
			::CloseHandle(pi.hThread);
			if (lpAtt)
			{
				delete lpAtt;
				lpAtt = NULL;
			}
		}
		else break;
	}
}

void IEBrowserHandler::CloseBrowser()
{
	if (m_hExitEvent)
	{
		SetEvent(m_hExitEvent);
		CloseHandle(m_hExitEvent);
		m_hExitEvent = NULL;
	}
	if(m_hHostWnd)
	{
		DestroyWindow(m_hHostWnd);
		m_hHostWnd = NULL;
	}
	if (m_hProcess)
	{
		TerminateProcess(m_hProcess, 0);
		CloseHandle(m_hProcess);
		m_hProcess = NULL;
	}
}

void IEBrowserHandler::ClearCache()
{
	/*
1    = Browsing History
2    = Cookies
4    = Temporary Internet Files
8    = Offline favorites and download history
16   = Form Data
32   = Passwords
64   = Phishing Filter Data
128  = Web page Recovery Data
256  = Do not Show GUI when running the cache clear
512  = Do not use Multi-threading for deletion
1024 = Valid only when browser is in private browsing mode
2048 = Tracking Data
4096 = Data stored by add-ons
8192 = Preserves Cached data for Favorite websites
	*/
	ShellExecute(NULL, L"open", L"rundll32.exe", L"InetCpl.cpl,ClearMyTracksByProcess 4351", NULL, SW_SHOWNORMAL);
	if (m_hIEWnd) PostMessage(m_hIEWnd, WM_USER + 1003, 0, 0);
	return;
	GROUPID groupId = 0;

	// Local variables
	DWORD cacheEntryInfoBufferSizeInitial = 0;
	DWORD cacheEntryInfoBufferSize = 0;

	INTERNET_CACHE_ENTRY_INFO *internetCacheEntry;
	HANDLE enumHandle = NULL;
	BOOL returnValue = false;

	
	enumHandle = FindFirstUrlCacheGroup(0, CACHEGROUP_SEARCH_ALL, 0, 0, &groupId, 0);

	// If there are no items in the Cache, you are finished.
	if (enumHandle != NULL && ERROR_NO_MORE_ITEMS == GetLastError())
		return ;

	// Loop through Cache Group, and then delete entries.
	if (enumHandle != NULL)
	{
		while (1)
		{
			// Delete a particular Cache Group.
			returnValue = DeleteUrlCacheGroup(groupId, CACHEGROUP_FLAG_FLUSHURL_ONDELETE, 0);

			returnValue = FindNextUrlCacheGroup(enumHandle, &groupId, 0);

			if (!returnValue)
			{
				break;
			}
		}
	}

	// Start to delete URLs that do not belong to any group.
	enumHandle = FindFirstUrlCacheEntry(NULL, 0, &cacheEntryInfoBufferSizeInitial);
	if (enumHandle == NULL && ERROR_NO_MORE_ITEMS == GetLastError())
		return ;

	cacheEntryInfoBufferSize = cacheEntryInfoBufferSizeInitial;
	internetCacheEntry = (INTERNET_CACHE_ENTRY_INFO*)malloc(cacheEntryInfoBufferSize);
	enumHandle = FindFirstUrlCacheEntry(NULL, internetCacheEntry, &cacheEntryInfoBufferSizeInitial);
	if (enumHandle != NULL)
	{
		while (1)
		{
			cacheEntryInfoBufferSizeInitial = cacheEntryInfoBufferSize;
			returnValue = DeleteUrlCacheEntry(internetCacheEntry->lpszSourceUrlName);

			// Allows try to get the next entry
			returnValue = FindNextUrlCacheEntry(enumHandle, internetCacheEntry, &cacheEntryInfoBufferSizeInitial);

			DWORD dwError = GetLastError();
			if (!returnValue && ERROR_NO_MORE_ITEMS == dwError)
			{
				break;
			}

			if (!returnValue && cacheEntryInfoBufferSizeInitial > cacheEntryInfoBufferSize)
			{
				cacheEntryInfoBufferSize = cacheEntryInfoBufferSizeInitial;
				internetCacheEntry = (INTERNET_CACHE_ENTRY_INFO*)realloc(internetCacheEntry, cacheEntryInfoBufferSize);
				returnValue = FindNextUrlCacheEntry(enumHandle, internetCacheEntry, &cacheEntryInfoBufferSizeInitial);
			}
		}
	}

	free(internetCacheEntry);
}

HWND IEBrowserHandler::GetHostWnd(bool bIE)
{
	if (bIE) return m_hIEWnd;
	else return m_hHostWnd;
}

int IEBrowserHandler::GetBrowserID()
{
	return (int)m_hProcess;
}

void IEBrowserHandler::SetMute(BOOL bMute)
{
	if (m_dwIEPid) 
	{
		if (!m_pAudioMgr) m_pAudioMgr = new CAudioMgr;
		m_bMute = bMute;
		m_pAudioMgr->SetProcessMute(m_dwIEPid, bMute);
		if(m_dwIEExtraPid) m_pAudioMgr->SetProcessMute(m_dwIEExtraPid, bMute);
	}
	
}

void IEBrowserHandler::UpdatePos(int nMarginLeft, int nMarginTop, int nMarginRight, int nMarginBotoom, bool bTopFull)
{
	if(!bTopFull)
	{
		if (!nMarginRight && !nMarginBotoom) {
			//move
			HWND hParent = GetParent(m_hHostWnd);
			HWND hAncestor = GetAncestor(hParent, GA_ROOT);
			RECT rcAncestor = { 0 };
			GetWindowRect(hAncestor, &rcAncestor);
			int nX = nMarginLeft + rcAncestor.left;
			int nY = nMarginTop + rcAncestor.top;
			SetWindowPos(m_hHostWnd, NULL, nX, nY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}
		else {
			HWND hParent = GetParent(m_hHostWnd);
			HWND hAncestor = GetAncestor(hParent, GA_ROOT);
			RECT rcAncestor = { 0 };
			GetWindowRect(hAncestor, &rcAncestor);
			RECT rcParent = { 0 };
			GetWindowRect(hParent, &rcParent);
			int nX = nMarginLeft + rcAncestor.left;
			int nY = nMarginTop + rcAncestor.top;
			SetWindowPos(m_hHostWnd, NULL, nX, nY, nMarginRight, nMarginBotoom, SWP_NOZORDER);
		}
	}
	if (m_hIEWnd) SendMessage(m_hIEWnd, WM_USER + 1002, 0, 0);
}

void IEBrowserHandler::OnBrowserCreated(HWND hIEWnd)
{
	if (hIEWnd)
	{
		m_hIEWnd = hIEWnd;
		std::string szBaseJson = client::MainContextImpl::Get()->GetBaseInfo();

		COPYDATASTRUCT data;
		data.dwData = 0;
		data.cbData = szBaseJson.size();
		data.lpData = (LPVOID)szBaseJson.c_str();
		SendMessage(m_hIEWnd, WM_COPYDATA, (WPARAM)GetCurrentProcessId(), (LPARAM)&data);
	}
}

bool IEBrowserHandler::IsMute()
{
	return m_bMute;
}

void IEBrowserHandler::OnRecvBroadCast(std::string szMsg)
{
	if(m_hIEWnd)
	{
		COPYDATASTRUCT data;
		data.dwData = 1;
		data.cbData = szMsg.size();
		data.lpData = (LPVOID)szMsg.c_str();
		SendMessage(m_hIEWnd, WM_COPYDATA, NULL, (LPARAM)&data);
	}
}

void IEBrowserHandler::BroadCast(std::string szMsg, int nTarget)
{
	int nBrowserID = (int)m_hProcess;
	CefRefPtr<client::RootWindow> rootWindow = client::MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(nBrowserID);
	if (rootWindow == NULL) return;

	std::string szCallerTag = rootWindow->GetBrowserTagFromID(nBrowserID);

	std::wstring wszMsg = UtilString::SA2W(szMsg);

	CefRefPtr<CefValue> value = CefParseJSON(wszMsg.c_str(), JSON_PARSER_RFC);
	if (value.get() && value->GetType() == VTYPE_DICTIONARY)
	{
		CefRefPtr<CefDictionaryValue> request_dict = value->GetDictionary();
		client::MainContextImpl::Get()->GetRootWindowManager()->BroadCastMsg(nBrowserID, szCallerTag, request_dict, nTarget);
	}
	
}

void IEBrowserHandler::BeforePopup(std::string szURL)
{
	int nBrowserID = (int)m_hProcess;
	CefRefPtr<client::RootWindow> rootWindow = client::MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(nBrowserID);
	if (rootWindow == NULL) return;

	std::string szCallerTag = rootWindow->GetBrowserTagFromID(nBrowserID);

	CefRefPtr<CefDictionaryValue> arg = CefDictionaryValue::Create();
	arg->SetInt("senderid", nBrowserID);
	arg->SetString("sendertag", szCallerTag);
	arg->SetString("targeturl", szURL);
	
	client::MainContextImpl::Get()->GetRootWindowManager()->PostCustomMsg(nBrowserID, szCallerTag, "onOpenURL", arg);
	
}

void IEBrowserHandler::OnTitleChange(std::string szTitle)
{
	if (m_pDelegate) m_pDelegate->OnIETitleChange(szTitle);
}

void IEBrowserHandler::NavTo(std::string szURL)
{
	if (m_hIEWnd)
	{
		COPYDATASTRUCT data;
		data.dwData = 2;
		data.cbData = szURL.size();
		data.lpData = (LPVOID)szURL.c_str();
		SendMessage(m_hIEWnd, WM_COPYDATA, NULL, (LPARAM)&data);
	}
}

void IEBrowserHandler::Reload(bool bWithCache)
{
	if (m_hIEWnd)
	{
		std::string szCache;
		if (bWithCache) szCache = "yes";
		else szCache = "no";
		COPYDATASTRUCT data;
		data.dwData = 3;
		data.cbData = szCache.size();
		data.lpData = (LPVOID)szCache.c_str();
		SendMessage(m_hIEWnd, WM_COPYDATA, NULL, (LPARAM)&data);
	}
}

void IEBrowserHandler::OnFlashError(std::string szType)
{
	int nBrowserID = (int)m_hProcess;
	CefRefPtr<client::RootWindow> rootWindow = client::MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(nBrowserID);
	std::string szCallerTag = rootWindow->GetBrowserTagFromID(nBrowserID);

	CefRefPtr<CefDictionaryValue> msg = CefDictionaryValue::Create();
	msg->SetString("type", szType);
	client::MainContextImpl::Get()->GetRootWindowManager()->PostCustomMsg(nBrowserID, szCallerTag, "onFlashError", msg);
}

void IEBrowserHandler::SetFixFlash(int nFix)
{
	m_nFixFlash = nFix;
}



void IEBrowserHandler::OnExtraProcessCreate()
{
// 	if (!m_dwIEExtraPid) return;
// 
// 	std::string szPath = RTCTX::CurPath();
// 	szPath += "\\PayProtect.dll";
// 
// 	TCHAR szShareData[64] = { 0 };
// 	swprintf_s(szShareData, L"PayProtect%d", m_dwIEExtraPid);
// 
// 	m_hShareData = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4, szShareData);
// 	if (m_hShareData)
// 	{
// 		m_pShareData = (char *)::MapViewOfFile(m_hShareData, FILE_MAP_ALL_ACCESS, 0, 0, 0);
// 		if (m_pShareData) memcpy(m_pShareData, &m_hHostWnd, 4);
// 	}
// 
// 	HMODULE hPlugInDLL = ((client::MainContextImpl*)client::MainContextImpl::Get())->GetPlugInDLL();
// 	if (hPlugInDLL)
// 	{
// 		typedef bool(*fPayProtect)(int nPID, const wchar_t* szInjectDLL);
// 		fPayProtect payProtect = (fPayProtect)GetProcAddress(hPlugInDLL, "PayProtect");
// 		if (payProtect) payProtect(m_dwIEExtraPid, UtilString::SA2W(szPath).c_str());
// 	}
}

void IEBrowserHandler::OnExtraProcessPay(std::string szURL)
{
	int nBrowserID = (int)m_hProcess;
	CefRefPtr<client::RootWindow> rootWindow = client::MainContextImpl::Get()->GetRootWindowManager()->GetWindowForBrowser(nBrowserID);
	std::string szCallerTag = rootWindow->GetBrowserTagFromID(nBrowserID);

	CefRefPtr<CefDictionaryValue> arg = CefDictionaryValue::Create();
	arg->SetInt("senderid", nBrowserID);
	arg->SetString("sendertag", szCallerTag);
	arg->SetString("targeturl", szURL);

	client::MainContextImpl::Get()->GetRootWindowManager()->PostCustomMsg(nBrowserID, szCallerTag, "onOpenURL", arg);
}
