#pragma once
#include <string>
#include "LayeredWindow.h"

class ToastWnd : public LayeredWindow
{
public:
	class Delegate {
	public:
		virtual void OnToastEnd(int nCallerId, std::wstring szMsg) {}
	};

	ToastWnd();
	~ToastWnd();
	void SetToastInfo(Delegate* pDelegate, int nCallerID,const std::wstring& szMsg, int nStayTime, int nFadeTime);

	virtual void OnTimer(int nTimerID);
	virtual void OnCompose(HDC hOSRDC, RECT rcPos);
	virtual void SetVisible(bool bVisible);
protected:
	float m_fAlpha;
	float m_fDelta;

	int m_nCallerID;
	int m_nStayTime;
	int m_nFadeTime;
	
	std::wstring m_szMsg;
	Delegate* m_pDelegate;
};

