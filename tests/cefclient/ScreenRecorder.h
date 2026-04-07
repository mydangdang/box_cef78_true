#pragma once
#include <wtypes.h>
#include <thread>
#include "browser\RecordBoundWnd.h"

class ScreenRecorder
{
public:
	static ScreenRecorder* Instance();
	bool Start(std::string szTagName,int nTime, int nRate, int nShrink, RECT rcPos);
	bool Stop();
	void OnEndRecord();
protected:
	void MoniteThread();
protected:
	HANDLE m_hMoniteThread = NULL;
	HANDLE m_hFFMPEGProcess = NULL;

	HANDLE hReadPipe = NULL;
	HANDLE hWritePipe = NULL;
	HANDLE hChildReadPipe = NULL;
	HANDLE hChildWritePipe = NULL;

	DWORD m_dwFFMPEGPID = 0;
	std::thread tMonitor;
	std::string m_szTagName;
	std::string m_szTmpMp4;
	RecordBoundWnd m_BoundWnd;
	static ScreenRecorder* m_pThis;
};

