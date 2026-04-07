#include "CBaseTrayIcon.h"

CBaseTrayIcon::CBaseTrayIcon()
{
    ZeroMemory(&m_nID,sizeof(NOTIFYICONDATA));
    m_nID.cbSize   = sizeof(NOTIFYICONDATA);
    m_nID.uFlags   = NIF_ICON;
}


CBaseTrayIcon::~CBaseTrayIcon()
{
    Delete();
}

// 显示托盘
BOOL CBaseTrayIcon::Show()
{
	if (m_nID.hIcon == NULL)
	{
		return FALSE;
	}

    return (0 != Shell_NotifyIcon(NIM_ADD,&m_nID));
}

// 删除托盘
BOOL CBaseTrayIcon::Delete()
{
	if (m_nID.hIcon == NULL)
	{
		return FALSE;
	}

	return (0 != Shell_NotifyIcon(NIM_DELETE,&m_nID));
}

// 修改托盘显示的图标
BOOL CBaseTrayIcon::ModifyIcon(HINSTANCE hInst,UINT idIcon)
{
    if(hInst == NULL)
        return FALSE;

    HICON hIcon = LoadIcon(hInst,MAKEINTRESOURCE(idIcon));
    m_nID.hIcon = hIcon;

    return (0 != Shell_NotifyIcon(NIM_MODIFY,&m_nID));
}

BOOL CBaseTrayIcon::ModifyIcon(HICON hIcon)
{
    m_nID.hIcon = hIcon;

    return (0 != Shell_NotifyIcon(NIM_MODIFY,&m_nID));
}

BOOL CBaseTrayIcon::ModifyIcon(HINSTANCE hInst,LPCTSTR lpszIcon)
{
    if(hInst == NULL)
        return FALSE;

    HICON hIcon = LoadIcon(hInst,lpszIcon);
    m_nID.hIcon = hIcon;

    return (0 != Shell_NotifyIcon(NIM_MODIFY,&m_nID));
}

// 设置接收托盘消息的窗口
BOOL CBaseTrayIcon::SetNotifyWnd(HWND hWnd,UINT uCallbackMsg)
{
    if(hWnd != NULL && IsWindow(hWnd))
    {
        m_nID.uFlags |= NIF_MESSAGE;
        m_nID.hWnd = hWnd;
        m_nID.uCallbackMessage = uCallbackMsg;

        return TRUE;
    }

    return FALSE;
}

// 设置托盘提示信息
void CBaseTrayIcon::SetToolTip(LPCTSTR lpszTip /*= NULL*/)
{
    if(lpszTip != NULL)
    {
        m_nID.uFlags |= NIF_TIP;
        _tcsncpy_s(m_nID.szTip, ARRAYSIZE(m_nID.szTip),lpszTip,ARRAYSIZE(m_nID.szTip)-1); 
    }
    else
    {
        m_nID.uFlags |= ~NIF_TIP;
    }

	Shell_NotifyIcon(NIM_MODIFY,&m_nID);
}

// 设置托盘气球提示
void CBaseTrayIcon::SetBallonToolTip(LPCTSTR lpszTitle /*= NULL*/,LPCTSTR lpszTip /*= NULL*/)
{
    m_nID.uFlags |= NIF_INFO;
	m_nID.dwInfoFlags = NIIF_USER;
    if(lpszTip != NULL)
    {
        _tcsncpy_s(m_nID.szInfo, ARRAYSIZE(m_nID.szInfo), lpszTip, ARRAYSIZE(m_nID.szTip)-1);

        if(lpszTitle != NULL)
        {
			_tcsncpy_s(m_nID.szInfoTitle, ARRAYSIZE(m_nID.szInfoTitle), lpszTitle, ARRAYSIZE(m_nID.szInfoTitle)-1);
        }
    }
    else
    {
        lstrcpy(m_nID.szInfo,_T("")); 
    }

	Shell_NotifyIcon(NIM_MODIFY,&m_nID);
}

// 设置托盘图标
BOOL CBaseTrayIcon::SetIcon(HINSTANCE hInst,UINT idIcon)
{
	if(hInst == NULL)
		return FALSE;

	HICON hIcon = LoadIcon(hInst,MAKEINTRESOURCE(idIcon));
	if(hIcon == NULL)
		return FALSE;

	m_nID.hIcon = hIcon;

	return TRUE;
}


