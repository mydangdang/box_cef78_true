#pragma  once
#include <Windows.h>
#include <tchar.h>
#include <ShellAPI.h>

class CBaseTrayIcon
{
public:
    CBaseTrayIcon();
    ~CBaseTrayIcon();

public:
    BOOL Show();
    BOOL Delete();

    BOOL ModifyIcon(HINSTANCE hInst,UINT idIcon);
    BOOL ModifyIcon(HICON hIcon);
    BOOL ModifyIcon(HINSTANCE hInst,LPCTSTR lpszIcon);

    BOOL SetNotifyWnd(HWND hWnd,UINT uCallbackMsg);

    void SetToolTip(LPCTSTR lpszTip = NULL);
    void SetBallonToolTip(LPCTSTR lpszTitle = NULL,LPCTSTR lpszTip = NULL);

    BOOL SetIcon(HINSTANCE hInst,UINT idIcon);

private:
    NOTIFYICONDATA m_nID;
};
