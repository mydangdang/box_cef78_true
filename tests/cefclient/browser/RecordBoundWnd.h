#pragma once
#include <string>
#include "LayeredWindow.h"

class RecordBoundWnd : public LayeredWindow
{
public:
	RecordBoundWnd();
	~RecordBoundWnd();
	void OnCompose(HDC hOSRDC, RECT rcPos);
};