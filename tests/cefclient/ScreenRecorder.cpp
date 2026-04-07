#include "ScreenRecorder.h"
#include <sstream>
#include "RTCTX.h"
#include <shlwapi.h>
#include <ShlObj.h>
#include "browser\main_context_impl.h"
#include "..\shared\browser\util_win.h"
#include "HotKeyManager.h"

ScreenRecorder* ScreenRecorder::m_pThis = NULL;

ScreenRecorder* ScreenRecorder::Instance()
{
	if (!m_pThis) m_pThis = new ScreenRecorder;
	return m_pThis;
}

void ScreenRecorder::MoniteThread()
{
	WaitForSingleObject(m_hFFMPEGProcess, -1);

	if (m_dwFFMPEGPID) client::MainContextImpl::Get()->OnRecordScreenStop(m_szTagName);
	else client::MainContextImpl::Get()->OnRecordScreenStop("");

	stHotKey* pHotKey = hotKeyManager.GetHotKey(HK_STOPRECORDSCREEN);
	if (pHotKey) hotKeyManager.DelHotKey(HK_STOPRECORDSCREEN);

	m_hFFMPEGProcess = NULL;
	m_hMoniteThread = NULL;
	m_dwFFMPEGPID = 0;
}

bool ScreenRecorder::Start(std::string szTagName,int nTime, int nRate,int nShrink, RECT rcPos)
{
	if (m_hMoniteThread) return false;
	m_szTagName = szTagName;
	//ffmpeg -f gdigrab -framerate 40 -offset_x 100 -offset_y 220 -video_size 800*600 -i desktop -s 800*600 -pix_fmt yuv420p -t 100 -y D:\out.mp4
	if (nShrink)
	{
		::InflateRect(&rcPos, -nShrink, -nShrink);
	}
	int nWidth = rcPos.right - rcPos.left;
	if (nWidth % 2) nWidth += 1;
	int nHeight = rcPos.bottom - rcPos.top;
	if (nHeight % 2) nHeight += 1;

	std::string szShowRegion;
#ifdef _DEBUG
	szShowRegion = " -show_region 1";
#endif

	std::string szExePath = RTCTX::CurPath() + "\\screenrecorder\\recorder.exe";
	if (!PathFileExistsA(szExePath.c_str())) return false;

	m_szTmpMp4 = client::DataPath() + "\\recordertmp.mp4";
	std::stringstream ss;
	ss << szExePath <<" -f gdigrab -framerate " << nRate << " -offset_x " << rcPos.left << " -offset_y " << rcPos.top
		<< " -video_size " << nWidth << "*" << nHeight << szShowRegion << " -i desktop -s "<< nWidth << "*" << nHeight <<" -pix_fmt yuv420p -t " << nTime << " -y " << m_szTmpMp4;

	CreatePipe(&hChildReadPipe, &hChildWritePipe, NULL, NULL);
	SetHandleInformation(hChildReadPipe, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	PROCESS_INFORMATION pi;
	LPSECURITY_ATTRIBUTES lpAtt = new SECURITY_ATTRIBUTES;
	lpAtt->bInheritHandle = TRUE;
	lpAtt->lpSecurityDescriptor = NULL;
	lpAtt->nLength = sizeof(SECURITY_ATTRIBUTES);
	STARTUPINFOA si = {sizeof(si)};
	::ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdInput = hChildReadPipe;
	si.wShowWindow = SW_HIDE;

	BOOL bRet = ::CreateProcessA(NULL, (LPSTR)ss.str().c_str(), lpAtt, NULL,TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	if (!bRet) return false;

	m_dwFFMPEGPID = pi.dwProcessId;
	m_hFFMPEGProcess = pi.hProcess;

	stHotKey* pHotKey = hotKeyManager.GetHotKey(HK_STOPRECORDSCREEN);
	if(!pHotKey) hotKeyManager.AddHotKey(HK_STOPRECORDSCREEN, VK_F6, 0);

	
	RECT rcBound = rcPos;
	if (nShrink > 5) InflateRect(&rcBound, 5, 5);
	m_BoundWnd.CreateLayeredWnd(NULL, rcBound);
	m_BoundWnd.SetVisible(true);
	

	tMonitor = std::thread(&ScreenRecorder::MoniteThread, this);
	m_hMoniteThread = tMonitor.native_handle();
	
	::CloseHandle(pi.hThread);
	if (lpAtt)
	{
		delete lpAtt;
		lpAtt = NULL;
	}
	return true;
}

bool ScreenRecorder::Stop()
{
	if (!m_dwFFMPEGPID) return false;
	DWORD dwWritted = 0;

	char* strBuffer = "q";

	if (!hChildWritePipe || !WriteFile(hChildWritePipe, strBuffer, 1, &dwWritted, 0))
	{
		AttachConsole(m_dwFFMPEGPID);
		SetConsoleCtrlHandler(NULL, TRUE);
		GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
	}
	
	DWORD dwRet = WaitForSingleObject(m_hMoniteThread, -1);
	if (dwRet != WAIT_OBJECT_0)
	{
		m_dwFFMPEGPID = 0;
		TerminateProcess(m_hFFMPEGProcess,0);
	}
	m_hFFMPEGProcess = NULL;
	m_hMoniteThread = NULL;
	m_dwFFMPEGPID = 0;
	return true;
}

void ScreenRecorder::OnEndRecord()
{
	m_BoundWnd.SetVisible(false);

	char path[255];
	SHGetSpecialFolderPathA(0, path, CSIDL_DESKTOPDIRECTORY, 0);
	char strFilename[MAX_PATH] = {0};
	sprintf_s(strFilename, "录制文件%I64d.mp4", time(NULL));
	OPENFILENAMEA ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.nFilterIndex = 1;
	ofn.lpstrFilter = "MP4 文件(*.mp4)";
	ofn.lpstrFile = strFilename;
	ofn.nMaxFile = sizeof(strFilename);
	ofn.lpstrInitialDir = path;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle = "保存到";
	if (GetSaveFileNameA(&ofn))
	{
		MoveFileA(m_szTmpMp4.c_str(), strFilename);
	}
	else
	{
		DeleteFileA(m_szTmpMp4.c_str());
	}
}
