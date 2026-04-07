#include "RecordBoundWnd.h"
#include "GdiPlusEnv.h"
#include "../string_util.h"


RecordBoundWnd::RecordBoundWnd()
{
	
}

RecordBoundWnd::~RecordBoundWnd()
{

}

void RecordBoundWnd::OnCompose(HDC hOSRDC, RECT rcPos)
{
	Graphics gs(hOSRDC);
	Pen redPen(Color(180, 232, 78, 64), 3);
	Rect rc(0, 0, rcPos.right - rcPos.left, rcPos.bottom - rcPos.top);
	gs.DrawRectangle(&redPen, rc);
}