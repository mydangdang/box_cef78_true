#include "TopFullWnd.h"
#include <process.h>
#include "IEKernel/src/utils/string_util.h"

const wchar_t* g_TopFullBrowserWnd = L"TopFullBrowserWnd";

TopFullWnd::TopFullWnd()
{
	m_bIsFullScreenState = false;
	m_hTopFullWnd = NULL;
	m_hInstance = GetModuleHandle(NULL);
	RegMainClass();
}

TopFullWnd::~TopFullWnd()
{
	
}


void TopFullWnd::SetWndFullTop(client::BrowserWindowStdWin *pBrowserWnd,HWND hOldParent,float fScal)
{
	m_hOldParent = hOldParent;
	int nWidth = GetSystemMetrics(SM_CXSCREEN);
	int nHeight = GetSystemMetrics(SM_CYSCREEN);
	if (!m_hTopFullWnd) 
	{
		std::wstring szTitle = L"游戏盒子";
		szTitle = UtilString::SU2W(pBrowserWnd->GetCurTitle());
		m_hTopFullWnd = CreateWindowEx(NULL, g_TopFullBrowserWnd, szTitle.c_str(),
			WS_POPUP,0, 0, nWidth, nHeight, NULL, NULL, m_hInstance, this);

		std::wstring szTipTxt = L"按F11退出全屏";
		RECT rcPos = { 0 };
		int nTxtWidth = m_TipWnd.GetTextWidth(szTipTxt.c_str());
		nTxtWidth += 30;
		nTxtWidth *= fScal;

		rcPos.left = (nWidth - nTxtWidth) / 2.0f;
		rcPos.top = 20* fScal;
		rcPos.right = rcPos.left + nTxtWidth;
		rcPos.bottom = rcPos.top + 50 * fScal;
		m_TipWnd.CreateLayeredWnd(m_hTopFullWnd, rcPos,true,false);
		m_TipWnd.SetToastInfo(NULL, 0, szTipTxt.c_str(), 2, 2);
	}
	if (!m_hTopFullWnd) return;

	m_bIsFullScreenState = true;
	m_OldWndInfo.pTargetBrowsreWnd = pBrowserWnd;
	pBrowserWnd->SetTopFull(true);

	
	m_OldWndInfo.hTarget = pBrowserWnd->GetWindowHandle();
	if (pBrowserWnd->IsTabBrowser() && !pBrowserWnd->IsIEKernel()) 
		m_OldWndInfo.hTarget = pBrowserWnd->GetBrowserHostWindowHandle();
	
	m_OldWndInfo.hParent = GetParent(m_OldWndInfo.hTarget);

	RECT rcOldPos;
	GetWindowRect(m_OldWndInfo.hTarget, &rcOldPos);
	
	m_OldWndInfo.nOldX = 0;
	m_OldWndInfo.nOldY = 0;
	m_OldWndInfo.nOldWidth = rcOldPos.right - rcOldPos.left;
	m_OldWndInfo.nOldHeight = rcOldPos.bottom - rcOldPos.top;

	SetParent(m_OldWndInfo.hTarget, m_hTopFullWnd);
	SetWindowPos(m_OldWndInfo.hTarget, NULL, 0, 0, nWidth, nHeight, SWP_NOZORDER);
	
	RECT rcParentPos = { 0 };
	GetWindowRect(m_OldWndInfo.hParent, &rcOldPos);
	if (rcOldPos.left > nWidth)
		SetWindowPos(m_hTopFullWnd, 0, nWidth, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	else
		SetWindowPos(m_hTopFullWnd, 0,	0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

	if(m_hOldParent) ShowWindow(m_hOldParent, SW_HIDE);

	m_TipWnd.SetVisible(true);
}

void TopFullWnd::RestoreWnd()
{
	if(m_bIsFullScreenState)
	{
		m_bIsFullScreenState = false;
		ShowWindow(m_hTopFullWnd, SW_HIDE);
		if (m_hOldParent) ShowWindow(m_hOldParent, SW_SHOW);
		SetParent(m_OldWndInfo.hTarget, m_OldWndInfo.hParent);
		SetWindowPos(m_OldWndInfo.hTarget, NULL, m_OldWndInfo.nOldX, m_OldWndInfo.nOldY, m_OldWndInfo.nOldWidth, m_OldWndInfo.nOldHeight, SWP_NOZORDER);
		m_OldWndInfo.pTargetBrowsreWnd->SetTopFull(false);
	}
}

bool TopFullWnd::IsFullScreen()
{
	return m_bIsFullScreenState;
}

void TopFullWnd::Hide()
{
	ShowWindow(m_hTopFullWnd, SW_HIDE);
}

void TopFullWnd::Show()
{
	ShowWindow(m_hTopFullWnd, SW_SHOW);
}

LRESULT WINAPI TopFullBrowserWnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCREATE:
	{
		CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		TopFullWnd* pObjPtr = reinterpret_cast<TopFullWnd*>(cs->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pObjPtr));
	}
	break;
	case WM_NCDESTROY:
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
	}
	break;
	case WM_SYSCOMMAND:
	{
		if (wParam == SC_CLOSE)
		{
			return 0;
		}
	}
	break;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


void TopFullWnd::RegMainClass()
{
	WNDCLASS wc;
	wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;
	wc.hInstance = m_hInstance;
	wc.lpfnWndProc = TopFullBrowserWnd;
	wc.lpszClassName = g_TopFullBrowserWnd;
	wc.lpszMenuName = NULL;
	wc.style = 0;
	RegisterClass(&wc);
}
