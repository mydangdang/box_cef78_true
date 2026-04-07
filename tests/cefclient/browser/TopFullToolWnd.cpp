#include "TopFullToolWnd.h"
#include "GdiPlusEnv.h"
#include <CommCtrl.h>

const wchar_t* g_TopFullToolWndClass = L"TopFullToolWndClass";

TopFullToolWnd::TopFullToolWnd()
{
	m_nY = 0;
	m_nOldWidth = 0;
	m_nOldHeight = 0;
	m_hWnd = NULL;
	m_hDcOffScreen = NULL;
	m_hBmpOld = NULL;
	m_hBmpOffScreen = NULL;
	m_hInstance = GetModuleHandle(NULL);
	m_hEraser = ::CreateSolidBrush(RGB(0, 0, 0));
	RegWndClass();
}

TopFullToolWnd::~TopFullToolWnd()
{
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
	if (m_hWnd)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
	}
}

void TopFullToolWnd::CreateLayeredWnd(RECT rcPos)
{
	if(!m_hWnd)
	{
		m_nY = rcPos.bottom - rcPos.top;
		DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
		OffsetRect(&rcPos, 0, -20);
		m_rcWnd = rcPos;
		m_hWnd = CreateWindowEx(dwExStyle, g_TopFullToolWndClass, g_TopFullToolWndClass,
			WS_POPUP|WS_VISIBLE, m_rcWnd.left, rcPos.top, m_rcWnd.right - m_rcWnd.left, rcPos.bottom-rcPos.top, NULL, NULL, m_hInstance, this);
	}
}

void TopFullToolWnd::SetVisible(bool bVisible)
{
	if (m_hWnd)
	{
		if (bVisible) ShowWindow(m_hWnd, SW_SHOW);
		else ShowWindow(m_hWnd, SW_HIDE);
	}
}

void TopFullToolWnd::OnPaint()
{
	PAINTSTRUCT ps;
	BeginPaint(m_hWnd, &ps);

	DWORD dwExStyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
	if ((dwExStyle & WS_EX_LAYERED) != 0x80000)
		SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, dwExStyle ^ WS_EX_LAYERED);

	if (m_hBmpOffScreen && m_nOldWidth && m_nOldHeight)
	{
		if (m_nOldWidth != m_rcWnd.right - m_rcWnd.left || m_nOldHeight != m_rcWnd.bottom - m_rcWnd.top)
		{
			::SelectObject(m_hDcOffScreen, m_hBmpOld);
			DeleteObject(m_hBmpOffScreen);
			m_hBmpOffScreen = NULL;
		}
	}

	if (!m_hBmpOffScreen)
	{
		m_nOldWidth = m_rcWnd.right - m_rcWnd.left;
		m_nOldHeight = m_rcWnd.bottom - m_rcWnd.top;
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

	::FillRect(m_hDcOffScreen, &ps.rcPaint, m_hEraser);


	RECT curPos = { 0 };
	::GetWindowRect(m_hWnd, &curPos);
	int nXDest = curPos.left;
	int nYDest = curPos.top;
	int nWidth = curPos.right - curPos.left;
	int nHeight = curPos.bottom - curPos.top;

	OnCompose(m_hDcOffScreen, curPos);

	SIZE szWindow = { nWidth, nHeight };
	POINT ptSrc = { 0, 0 };
	POINT pt = { nXDest, nYDest };
	BLENDFUNCTION blendPixelFunction = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	::UpdateLayeredWindow(m_hWnd, NULL, &pt, &szWindow, m_hDcOffScreen, &ptSrc, 0, &blendPixelFunction, ULW_ALPHA);

	EndPaint(m_hWnd, &ps);
}

void TopFullToolWnd::OnCreate()
{

}

void TopFullToolWnd::OnTimer(int nTimerID)
{

}

void TopFullToolWnd::OnCompose(HDC hOSRDC, RECT rcPos)
{
	LPTSTR szMsg = L"ÍË³ö";
	Graphics gs(hOSRDC);
	int nWidth = rcPos.right - rcPos.left;
	int nHeight = rcPos.bottom - rcPos.top;
	Font font(L"Î¢ÈíÑÅºÚ", 13);
	RectF layoutRect(0, 0, 500, 500);
	RectF boundRect;
	gs.MeasureString(szMsg, wcslen(szMsg), &font, layoutRect, &boundRect);
	float fX = (nWidth - boundRect.Width) / 2.0f;
	float fY = (nHeight - boundRect.Height) / 2.0f + rcPos.top;

	boundRect.X = fX;
	boundRect.Y = fY;
	boundRect.Inflate(15.0f, 10.0f);

	SolidBrush bkBrush(Color(255, 1, 1, 1));
	Rect rcBk(rcPos.left, rcPos.top, rcPos.right - rcPos.left, rcPos.bottom - rcPos.top);
	gs.FillRectangle(&bkBrush, rcBk);

	HRGN rgnBk = CreateRoundRectRgn(boundRect.X, boundRect.Y, boundRect.X + boundRect.Width, boundRect.Y + boundRect.Height, 10,10);
	Region *grgn;
	grgn = Region::FromHRGN(rgnBk);
	SolidBrush fontBkBrush(Color(255, 25, 25, 25));
	gs.FillRegion(&fontBkBrush, grgn);
	::DeleteObject(rgnBk);

	SolidBrush fontBrush(Color(255, 255, 255, 255));
	gs.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
	gs.DrawString(szMsg, wcslen(szMsg), &font,PointF(fX, fY), &fontBrush);
}

BOOL g_bMouseTrack = FALSE;

LRESULT WINAPI TopFullToolWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCREATE:
	{
		CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		TopFullToolWnd* pObjPtr = reinterpret_cast<TopFullToolWnd*>(cs->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pObjPtr));
	}
	break;
	case WM_NCDESTROY:
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
	}
	break;
	case WM_CREATE:
	{
		TopFullToolWnd* pThis = (TopFullToolWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		pThis->OnCreate();
	}
		break;
	case WM_PAINT:
	{
		TopFullToolWnd* pThis = (TopFullToolWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		pThis->OnPaint();
		return 0;
	}
		break;
	case WM_TIMER:
	{
		TopFullToolWnd* pThis = (TopFullToolWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		pThis->OnTimer(int(wParam));
	}
	break;
	case WM_ERASEBKGND:
		return 1;
	case WM_NCMOUSEMOVE:

		break;
	case WM_MOUSEMOVE:
	{
		if (!g_bMouseTrack)
		{
			TRACKMOUSEEVENT tme = { 0 };
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_HOVER | TME_LEAVE ;
			tme.hwndTrack = hWnd;
			tme.dwHoverTime =  400UL;
			_TrackMouseEvent(&tme);
			g_bMouseTrack = true;
		
		}
	}
		break;
	case WM_MOUSELEAVE:
	{
		g_bMouseTrack = FALSE;
		
	}
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void TopFullToolWnd::RegWndClass()
{
	WNDCLASS wc;
	wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;
	wc.hInstance = m_hInstance;
	wc.lpfnWndProc = TopFullToolWndProc;
	wc.lpszClassName = g_TopFullToolWndClass;
	wc.lpszMenuName = NULL;
	wc.style = 0;
	RegisterClass(&wc);
}
