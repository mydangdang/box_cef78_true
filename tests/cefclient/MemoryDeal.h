#pragma once
#include <vector>
#include <string>
#include "windows.h"
#include "ipc/PipeServer.h"

struct stFreeTask
{
	DWORD dwPID;
	DWORD dwLastFree;
	HANDLE hProcess;
	stFreeTask()
	{
		dwPID = 0;
		dwLastFree = 0;
		hProcess = NULL;
	}
};

/*
* 改成由 JS 控制开始 和 每次 间隔时间
*/

class MemoryDeal : public IPipeServerListener
{
public:
	MemoryDeal();
	~MemoryDeal();

	void Init(bool bSrv = true);
	void Start(int nCheckInterval, int nFreeInterval);
	void FreeMeme();
	void OnAsyncRequest(string strRequest);
	void CheckThread();
	float GetMemSize();
	void MonitePID(DWORD dwPID,HANDLE hProcess = NULL);
	void AddFreeOBJ(int nBrowser);
	void DelFreeOBJ(int nBrowser);
	bool HasFreeOBJ(int nBrowser);
public:
	CLightLock m_Lock;
	float m_fMemSize = 0.0f;
	DWORD m_dwFreeInterval = 10000;
	DWORD m_dwCheckInterval = 3000;
	HANDLE m_hExitEvent = NULL;
	HANDLE m_hCheckThread = NULL;
	CPipeServer* m_pPipSrv = NULL;
	std::vector<int> m_vBrowser;
	std::vector<stFreeTask> m_vTasks;
};

extern MemoryDeal MemoryDealer;
