#pragma once
#include "browser\LayeredWindow.h"
#include <string>
#include "browser\GdiPlusEnv.h"

class InitialWnd : public LayeredWindow
{
public:
	InitialWnd();
	~InitialWnd();

	void CreateInitalWnd();
	
	virtual void OnCreate(HWND hWnd);
	virtual void OnTimer(int nTimerID);
	virtual void OnCompose(HDC hOSRDC, RECT rcPos);
	std::string GetCfg();
	void DoModal();

	static std::string m_szCfg;
protected:
	int m_nWave;
	int m_nInterval;
	int m_nDuration;
	float m_fDelta;
	float m_fRadiu;
	float m_fOffset;

	float m_fX;
	float m_fY;

	Bitmap* m_pBkImg;
	int m_nWidth;
	int m_nHeight;
};

