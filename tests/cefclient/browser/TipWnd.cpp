#include "TipWnd.h"
#include "GdiPlusEnv.h"
#include "../string_util.h"
#include <winuser.h>

#define TIME_TO_FADE 1001
#define TIME_FADING 1002

TipWnd::TipWnd()
{
	m_nCallerID = 1;
	m_fAlpha = 100.0f;
	m_pDelegate = 0;
}

TipWnd::~TipWnd()
{

}

void TipWnd::SetToastInfo(Delegate* pDelegate,int nCallerID,const std::wstring& szMsg, int nStayTime, int nFadeTime)
{
	m_szMsg = szMsg;
	m_pDelegate = pDelegate;
	m_nCallerID = nCallerID;
	m_nStayTime = nStayTime;
	m_nFadeTime = nFadeTime;
}

int TipWnd::GetTextWidth(std::wstring szText) 
{
	Graphics gs(GetDC(m_hWnd));
	Font font(L"Î¢ÈíÑÅºÚ", 12);
	RectF layoutRect(0, 0, 500, 500);
	RectF boundRect;
	gs.MeasureString(szText.c_str(), szText.length(), &font, layoutRect, &boundRect);
	return boundRect.Width;
}

void TipWnd::OnCompose(HDC hOSRDC, RECT rcPos)
{
	if (!m_szMsg.empty())
	{
		Graphics gs(hOSRDC);
		int nWidth = rcPos.right - rcPos.left;
		int nHeight = rcPos.bottom - rcPos.top;
		Font font(L"Î¢ÈíÑÅºÚ", 12);
		RectF layoutRect(0, 0, 500, 500);
		RectF boundRect;
		gs.MeasureString(m_szMsg.c_str(), m_szMsg.length(), &font, layoutRect, &boundRect);
		float fX = (nWidth - boundRect.Width) / 2.0f;
		float fY = (nHeight - boundRect.Height) / 2.0f;

		boundRect.X = fX;
		boundRect.Y = fY;
		boundRect.Inflate(5.0f, 5.0f);

		HRGN rgnBk = CreateRoundRectRgn(boundRect.X, boundRect.Y, boundRect.X + boundRect.Width, boundRect.Y + boundRect.Height, 5, 5);
		Region* grgn;
		grgn = Region::FromHRGN(rgnBk);
		SolidBrush fontBkBrush(Color(180 * m_fAlpha / 100.0f, 25, 25, 25));
		gs.FillRegion(&fontBkBrush, grgn);
		::DeleteObject(rgnBk);

		SolidBrush fontBrush(Color(255 * m_fAlpha / 100.0f, 255, 255, 255));
		gs.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
		gs.DrawString(m_szMsg.c_str(), m_szMsg.length(), &font,
			PointF(fX, fY), &fontBrush);
	}
}

void TipWnd::SetVisible(bool bVisible)
{
	if (m_hWnd)
	{
		if (bVisible)
		{
			KillTimer(m_hWnd, TIME_FADING);
			KillTimer(m_hWnd, TIME_TO_FADE);
			m_fAlpha = 100.0f;
			ShowWindow(m_hWnd, SW_SHOW);
			SetTimer(m_hWnd, TIME_TO_FADE, m_nStayTime * 1000, NULL);
			InvalidateRect(m_hWnd, NULL, FALSE);
		}
		else ShowWindow(m_hWnd, SW_HIDE);
	}
}

void TipWnd::OnTimer(int nTimerID)
{
	if (nTimerID == TIME_TO_FADE)
	{
		KillTimer(m_hWnd, nTimerID);
		UINT uElapse = 20;
		m_fDelta = 100.0f / (m_nFadeTime * (1000 / uElapse));
		SetTimer(m_hWnd, TIME_FADING, uElapse, NULL);
	}
	else if (nTimerID == TIME_FADING)
	{
		if (m_fAlpha > 0)
			m_fAlpha -= m_fDelta;
		else
		{
			m_fAlpha = 0;
			KillTimer(m_hWnd, nTimerID);
			ShowWindow(m_hWnd, SW_HIDE);
			if (m_pDelegate) m_pDelegate->OnToastEnd(m_nCallerID, m_szMsg);
		}
		InvalidateRect(m_hWnd, NULL, FALSE);
	}
}
