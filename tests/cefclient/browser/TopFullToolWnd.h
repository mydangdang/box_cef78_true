#pragma once
#include <windows.h>

class TopFullToolWnd
{
public:
	TopFullToolWnd();
	virtual ~TopFullToolWnd();

	void CreateLayeredWnd(RECT rcPos);
	virtual void SetVisible(bool bVisible);
	virtual void OnPaint();
	virtual void OnCreate();
	virtual void OnTimer(int nTimerID);
	virtual void OnCompose(HDC hOSRDC,RECT rcPos);
protected:
	void RegWndClass();
protected:
	int m_nY;
	int m_nOldWidth;
	int m_nOldHeight;
	HWND m_hWnd;
	HWND m_hParent;
	RECT m_rcWnd;
	HDC m_hDcOffScreen;
	HBRUSH m_hEraser;
	HBITMAP m_hBmpOld;
	HBITMAP m_hBmpOffScreen;
	HINSTANCE m_hInstance;
};

