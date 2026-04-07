#include "ShutDownPlan.h"
#include <process.h>

ShutDownPlan::ShutDownPlan()
{
	m_bPromoted = false;
	m_nLeftMinites = 0;
	m_nWarningMinites = 0;
	m_pObserver = NULL;
	m_hCancelEvent = CreateEvent(NULL,TRUE,FALSE,0);
	m_hCountingThread = 0;
}


ShutDownPlan::~ShutDownPlan()
{
	CancelPlan();
	TerminateCountingThread();
	if (m_hCancelEvent)
	{
		CloseHandle(m_hCancelEvent);
		m_hCancelEvent = NULL;
	}
}

BOOL ShutDownPlan::EnableShutdownPrivilege()
{
	HANDLE hProcess = NULL;
	HANDLE hToken = NULL;
	LUID uID = { 0 };
	TOKEN_PRIVILEGES stToken_Privileges = { 0 };
	hProcess = ::GetCurrentProcess();
	if (!::OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken)) 
		return FALSE;
	if (!::LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &uID))
		return FALSE;
	stToken_Privileges.PrivilegeCount = 1; 
	stToken_Privileges.Privileges[0].Luid = uID; 
	stToken_Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!::AdjustTokenPrivileges(hToken, FALSE, &stToken_Privileges, sizeof stToken_Privileges, NULL, NULL))
		return FALSE;
	if (::GetLastError() != ERROR_SUCCESS)
		return FALSE;
	::CloseHandle(hToken);
	return TRUE;
}

bool ShutDownPlan::StartPlan(Delegate* pObserver, int nBrowserID, int nLeftMinites, int nWarningMinites)
{
	if (!m_bPromoted && !EnableShutdownPrivilege()) return false;
	m_bPromoted = true;
	CancelPlan();
	TerminateCountingThread();
	m_nBrowserID = nBrowserID;
	m_pObserver = pObserver;
	m_nLeftMinites = nLeftMinites;
	m_nWarningMinites = nWarningMinites;
	if(m_hCancelEvent) ResetEvent(m_hCancelEvent);
	if (!m_hCountingThread) m_hCountingThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned(_stdcall *) (void *))ThreadCounting, (void *)this, 0, NULL);
	return true;
}

void ShutDownPlan::CancelPlan()
{
	if (m_hCancelEvent) SetEvent(m_hCancelEvent);
}

ShutDownPlan* ShutDownPlan::Instance()
{
	if (!m_pThis) m_pThis = new ShutDownPlan;
	return m_pThis;
}

void ShutDownPlan::Free()
{
	if (m_pThis)
	{
		delete m_pThis;
		m_pThis = NULL;
	}
}

bool ShutDownPlan::TerminateCountingThread()
{
	if (m_hCountingThread)
	{
		WaitForSingleObject(m_hCountingThread, -1);
		CloseHandle(m_hCountingThread);
		m_hCountingThread = NULL;
	}
	return true;
}

unsigned int ShutDownPlan::InnerThreadCounting()
{
	while (m_nLeftMinites && WaitForSingleObject(m_hCancelEvent, 60000) == WAIT_TIMEOUT)
	{
		m_nLeftMinites -= 1;
		if ( m_nLeftMinites == m_nWarningMinites && m_pObserver)
			m_pObserver->OnPlanWarning(m_nBrowserID, m_nWarningMinites);
	}

	if (m_nLeftMinites <= 0)
	{
		ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
	}
	return 0;
}

unsigned int __stdcall ShutDownPlan::ThreadCounting(void* lParam)
{
	ShutDownPlan *pThis = (ShutDownPlan*)lParam;
	if (pThis) pThis->InnerThreadCounting();
	return 0;
}

ShutDownPlan* ShutDownPlan::m_pThis = NULL;
