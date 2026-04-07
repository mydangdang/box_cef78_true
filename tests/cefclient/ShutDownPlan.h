#pragma once
#include <windows.h>

class ShutDownPlan
{
public:
	class Delegate
	{
	public:
		virtual void OnPlanWarning(int nBrowserID, int nLeftMinites) = 0;
	};
public:
	ShutDownPlan();
	~ShutDownPlan();

	bool StartPlan(Delegate* pObserver,int nBrowserID, int nLeftMinites, int nWarningMinites);
	void CancelPlan();

	static ShutDownPlan* Instance();
	static void Free();
protected:
	BOOL EnableShutdownPrivilege();
	bool TerminateCountingThread();
	unsigned int InnerThreadCounting();
	static unsigned int __stdcall ThreadCounting(void* lParam);
protected:
	bool m_bPromoted;
	int m_nBrowserID;
	int m_nLeftMinites;
	int m_nWarningMinites;
	Delegate* m_pObserver;
	HANDLE m_hCancelEvent;
	HANDLE m_hCountingThread;
	static ShutDownPlan* m_pThis;
};
