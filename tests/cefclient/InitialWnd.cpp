#include "InitialWnd.h"
#include <thread>
#include "resources\win\resource.h"
#include "..\shared\browser\util_win.h"
#include "WinHttpClient.h"
#include "string_util.h"
#include "RTCTX.h"

std::string InitialWnd::m_szCfg;

InitialWnd::InitialWnd()
{
	m_nWave = 30;
	m_fDelta = 0.0f;
	m_fRadiu = 0.0f;
	m_fOffset = 0.0f;
	m_nInterval = 50;
	m_nDuration = 360*50;
	m_pBkImg = NULL;

	m_fX = 0.0f;
	m_fY = 0.0f;
	m_nWidth = 0;
	m_nHeight = 0;
}


InitialWnd::~InitialWnd()
{
}

void InitialWnd::CreateInitalWnd()
{
	std::string szCurPath = RTCTX::CurPath();
	szCurPath += "\\staticres\\init.png";
	m_pBkImg = new Bitmap(UtilString::SA2W(szCurPath).c_str());
	if (!m_pBkImg) return;

	int nImgWidth = m_pBkImg->GetWidth();
	int nImgHeight = m_pBkImg->GetHeight();
	m_nWidth = nImgWidth *2;
	m_nHeight = nImgHeight *2;

	m_fX = (m_nWidth - nImgWidth) / 2.0f;
	m_fY = (m_nHeight-nImgHeight) / 2.0f;

	int	nLeft = (int)((GetSystemMetrics(SM_CXSCREEN) - m_nWidth) * 1.0f / 2);
	
	int	nTop = (int)((GetSystemMetrics(SM_CYSCREEN) - m_nHeight) * 1.0f / 2);
	RECT rcPos = { nLeft,nTop,nLeft + m_nWidth,nTop + m_nHeight };
	LayeredWindow::CreateLayeredWnd(NULL, rcPos);
}

void RequestCfg(HWND hNotify)
{
	wchar_t szTemp[512] = { 0 };
	int nCnts = LoadString(GetModuleHandle(NULL), 110, szTemp, 512);
	std::wstring szURL = std::wstring(szTemp, nCnts);

	DWORD dwBegin = time(NULL);

	int nRetry = 3;
	do 
	{
		WinHttpClient clt(szURL.c_str());
		clt.SendHttpRequest();
		wstring szContet = clt.GetResponseContent();
		if (!szContet.empty())
		{
			InitialWnd::m_szCfg = UtilString::SW2A(szContet);
			break;
		}
		else
		{
			nRetry--;
		}
	} while (nRetry > 0);

	if (InitialWnd::m_szCfg.empty())
	{
		InitialWnd::m_szCfg = R"({"url":"http://html.landers.yee5.com/pages/index","left":-1,"top":-1,"width":1400,"height":860,"tag":"indexWin"})";
	}
	
	DWORD dwEnd = time(NULL);

	if (dwEnd - dwBegin <= 1000)
		Sleep(1000 - (dwEnd - dwBegin));

	KillTimer(hNotify, 1001);
	PostMessage(hNotify, WM_CLOSE, 0, 0);
	PostMessage(hNotify, WM_QUIT, 0,0);
}

void InitialWnd::OnCreate(HWND hWnd)
{
	std::thread requestThread(RequestCfg, hWnd);
	requestThread.detach();
	RECT rcPos = { 0,0,m_nWidth,m_nHeight };


	SetTimer(hWnd, 1001, 20, NULL);
	m_fDelta = 12;
}

void InitialWnd::OnTimer(int nTimerID)
{
	if (m_fRadiu >= 360.0f) m_fRadiu = 0.0f;
	else m_fRadiu += m_fDelta;

	m_fOffset = sin(m_fRadiu / 360 * (3.1415926));
	InvalidateRect(m_hWnd, NULL, 0);
}

void InitialWnd::OnCompose(HDC hOSRDC, RECT rcPos)
{
	if(m_pBkImg)
	{
		Graphics gs(hOSRDC);
		gs.DrawImage(m_pBkImg, m_fX, m_fY + m_nWave*m_fOffset , m_pBkImg->GetWidth(), m_pBkImg->GetHeight());
	}
}

std::string InitialWnd::GetCfg()
{
	return m_szCfg;
}

void InitialWnd::DoModal()
{
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
