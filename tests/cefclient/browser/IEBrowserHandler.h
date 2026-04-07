#pragma once
#include <string>
#include <windows.h>
#include "include/cef_base.h"
#include "AudioManager.h"

class IEBrowserHandler
{
public:
	class IEDelegate
	{
	public:
		virtual void OnIETitleChange(std::string szTitle) = 0;
		virtual void OnIEProcessDeamon(int nNewBrowserID) = 0;
	};
public:
	IEBrowserHandler(IEDelegate* pDelegate,std::string szURL);
	~IEBrowserHandler();

	void RegMainClass();
	bool CreateBrowser(HWND hParent, const CefRect& rcMargin);
	void CloseBrowser();
	void ClearCache();

	HWND GetHostWnd(bool bIE = false);
	int	 GetBrowserID();
	void SetMute(BOOL bMute);

	void UpdatePos(int nMarginLeft,int nMarginTop,int nMarginRight,int nMarginBotoom,bool bTopFull);
	void OnBrowserCreated(HWND hIEWnd);
	bool IsMute();
	void OnRecvBroadCast(std::string szMsg);
	void BroadCast(std::string szMsg,int nTarget);
	void BeforePopup(std::string szURL);

	void OnTitleChange(std::string szTitle);
	void NavTo(std::string szURL);
	void Reload(bool bWithCache);
	void OnFlashError(std::string szType);
	void SetFixFlash(int nFix);
	void OnExtraProcessCreate();
	void OnExtraProcessPay(std::string szURL);

	void Deamon(std::string szPath, std::string szCmd, HANDLE hProcess, HANDLE hExitEvent);
protected:
	int m_nFixFlash;
	bool m_bMute;
	char *m_pShareData;
	HWND m_hIEWnd;
	HWND m_hHostWnd;
	DWORD m_dwIEPid;
	HANDLE m_hExitEvent;
	HANDLE m_hProcess;
	HANDLE m_hShareData;
	CAudioMgr *m_pAudioMgr;
	HINSTANCE m_hInstance;
	std::string m_szURL;
	IEDelegate *m_pDelegate;
public:
	DWORD m_dwIEExtraPid;
};

