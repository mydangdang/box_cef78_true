#pragma once
#include <windows.h>
#include "tests/cefclient/browser/browser_window_std_win.h"
#include "TipWnd.h"

struct WndInfo 
{
	int nOldX;
	int nOldY;
	int nOldWidth;
	int nOldHeight;
	HWND hTarget;
	HWND hParent;
	client::BrowserWindowStdWin *pTargetBrowsreWnd;
};

class TopFullWnd
{
public:
	TopFullWnd();
	~TopFullWnd();

	void SetWndFullTop(client::BrowserWindowStdWin *pBrowserWnd,HWND hOldParent,float fScal = 1.0f);
	void RestoreWnd();
	bool IsFullScreen();
	void Hide();
	void Show();
protected:
	void RegMainClass();
protected:
	bool m_bIsFullScreenState;
	HWND m_hOldParent;
	HWND m_hTopFullWnd;
	WndInfo m_OldWndInfo;
	TipWnd m_TipWnd;
	HINSTANCE m_hInstance;
};

