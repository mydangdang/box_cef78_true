#pragma once
#include <windows.h>

class LayeredWindow
{
public:
	LayeredWindow();
	virtual ~LayeredWindow();

	void CreateLayeredWnd(HWND hParent, RECT rcPos,bool bToolWind = true,bool bMargin = true);
	virtual void SetVisible(bool bVisible);
	bool IsVisible();
	void UpdatePos();
	virtual void OnPaint();
	virtual void OnCreate(HWND hWnd);
	virtual void OnTimer(int nTimerID);
	virtual void OnCompose(HDC hOSRDC,RECT rcPos);
protected:
	void RegWndClass();
protected:
	bool m_bMargin;
	int m_nOldWidth;
	int m_nOldHeight;
	HWND m_hWnd;
	HWND m_hParent;
	RECT m_rcWnd;
	RECT m_rcPos;
	HDC m_hDcOffScreen;
	HBRUSH m_hEraser;
	HBITMAP m_hBmpOld;
	HBITMAP m_hBmpOffScreen;
	HINSTANCE m_hInstance;
};

