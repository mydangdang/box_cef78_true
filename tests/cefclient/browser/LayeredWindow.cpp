#include "LayeredWindow.h"

const wchar_t* g_LayerdWndClass = L"LayeredWndClass";

LayeredWindow::LayeredWindow()
{
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

LayeredWindow::~LayeredWindow()
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

void LayeredWindow::CreateLayeredWnd(HWND hParent, RECT rcPos, bool bToolWind, bool bMargin)
{
	if(!m_hWnd)
	{
		m_bMargin = bMargin;
		m_hParent = hParent;
		DWORD dwExStyle = 0;
		if (bToolWind) dwExStyle |= WS_EX_TOOLWINDOW;
		m_rcPos = rcPos;

		RECT rcWnd = rcPos;
		if(hParent && bMargin)
		{
			RECT rcParent = { 0 };
			GetWindowRect(m_hParent, &rcParent);
			rcWnd = { rcParent.left + m_rcPos.left,rcParent.top + m_rcPos.top,rcParent.right - m_rcPos.right,rcParent.bottom - m_rcPos.bottom };
		}

		m_rcWnd = rcWnd;
		m_hWnd = CreateWindowEx(dwExStyle, g_LayerdWndClass, g_LayerdWndClass,
			WS_CHILDWINDOW | WS_POPUP, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top, hParent, NULL, m_hInstance, this);
	}
}

void LayeredWindow::SetVisible(bool bVisible)
{
	if (m_hWnd)
	{
		if (bVisible) ShowWindow(m_hWnd, SW_SHOW);
		else ShowWindow(m_hWnd, SW_HIDE);
	}
}

bool LayeredWindow::IsVisible()
{
	if (!m_hWnd) return false;
	return IsWindowVisible(m_hWnd);
}

void LayeredWindow::UpdatePos()
{
	if (m_hWnd && m_hParent && m_bMargin)
	{
		RECT rcParent = { 0 };
		GetWindowRect(m_hParent, &rcParent);
		RECT rcWnd = { rcParent.left + m_rcPos.left,rcParent.top + m_rcPos.top,rcParent.right - m_rcPos.right,rcParent.bottom - m_rcPos.bottom };
		
		m_rcWnd = rcWnd;
		SetWindowPos(m_hWnd, NULL, m_rcWnd.left, m_rcWnd.top, m_rcWnd.right - m_rcWnd.left, m_rcWnd.bottom - m_rcWnd.top, SWP_NOZORDER);
		
		if (m_nOldWidth != m_rcWnd.right - m_rcWnd.left || m_nOldHeight != m_rcWnd.bottom - m_rcWnd.top)
			InvalidateRect(m_hWnd, 0, true);
	}
}

void LayeredWindow::OnPaint()
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
	RECT rc = { 0, 0, m_rcWnd.right - m_rcWnd.left, m_rcWnd.bottom - m_rcWnd.top };
	::FillRect(m_hDcOffScreen, &rc, m_hEraser);


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

void LayeredWindow::OnCreate(HWND hWnd)
{

}

void LayeredWindow::OnTimer(int nTimerID)
{

}

void LayeredWindow::OnCompose(HDC hOSRDC, RECT rcPos)
{

}

LRESULT WINAPI LayeredWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCREATE:
	{
		CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		LayeredWindow* pObjPtr = reinterpret_cast<LayeredWindow*>(cs->lpCreateParams);
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
		LayeredWindow* pThis = (LayeredWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		pThis->OnCreate(hWnd);
	}
		break;
	case WM_PAINT:
	{
		LayeredWindow* pThis = (LayeredWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		pThis->OnPaint();
		return 0;
	}
		break;
	case WM_TIMER:
	{
		LayeredWindow* pThis = (LayeredWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		pThis->OnTimer(int(wParam));
	}
	break;
	case WM_ERASEBKGND:
		return 1;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void LayeredWindow::RegWndClass()
{
	WNDCLASS wc;
	wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;
	wc.hInstance = m_hInstance;
	wc.lpfnWndProc = LayeredWindowProc;
	wc.lpszClassName = g_LayerdWndClass;
	wc.lpszMenuName = NULL;
	wc.style = 0;
	RegisterClass(&wc);
}
