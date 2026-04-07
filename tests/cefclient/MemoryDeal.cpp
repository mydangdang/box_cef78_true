#include "MemoryDeal.h"
#include "ipc/PipeClient.h"
#include <Psapi.h>

#define IPC_PIPE_NAME "\\\\.\\pipe\\WebBox_memoryMonit"

MemoryDeal MemoryDealer;

MemoryDeal::MemoryDeal()
{

}

MemoryDeal::~MemoryDeal()
{
	if (m_hExitEvent)
	{
		SetEvent(m_hExitEvent);
		CloseHandle(m_hExitEvent);
		m_hExitEvent = NULL;
	}
	if (m_hCheckThread)
	{
		WaitForSingleObject(m_hCheckThread, 5000);
		CloseHandle(m_hCheckThread);
		m_hCheckThread = NULL;
	}
}

void MemoryDeal::Init(bool bSrv)
{
	if(bSrv)
	{
		m_pPipSrv = new CPipeServer(this);
		m_pPipSrv->InitPipeServer(IPC_PIPE_NAME);
		MonitePID(GetCurrentProcessId());
		if (!m_hExitEvent) m_hExitEvent = CreateEvent(NULL, TRUE, FALSE, 0);
	}
	else
	{
		CPipeClient sender(IPC_PIPE_NAME);
		sender.PostReuest(std::to_string(GetCurrentProcessId()));
	}
}

UINT __stdcall CheckThreadBridge(PVOID ctx)
{
	MemoryDeal* pThis = (MemoryDeal*)ctx;
	pThis->CheckThread();
	return 0;
}


void MemoryDeal::Start(int nCheckInterval, int nFreeInterval)
{
	m_dwFreeInterval = nFreeInterval * 1000;
	m_dwCheckInterval = nCheckInterval * 1000;
	if (!m_hCheckThread) m_hCheckThread = (HANDLE)_beginthreadex(NULL, 0, CheckThreadBridge, this, 0, NULL);
}

void MemoryDeal::OnAsyncRequest(string strRequest)
{
	MonitePID(atoi(strRequest.c_str()));
}

void MemoryDeal::MonitePID(DWORD dwPID, HANDLE hProcess)
{
	stFreeTask task;
	task.dwPID = dwPID;
	task.dwLastFree = GetTickCount();
	if (!hProcess) task.hProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA, FALSE, dwPID);
	else task.hProcess = hProcess;
	if(task.hProcess)
	{
		m_Lock.Lock();
		m_vTasks.push_back(task);
		m_Lock.UnLock();
	}
}

void MemoryDeal::AddFreeOBJ(int nBrowser)
{
// 	char szOut[256] = { 0 };
// 	sprintf_s(szOut, 256, "ADD OBJ %d \n", nBrowser);
// 	OutputDebugStringA(szOut);
	m_vBrowser.push_back(nBrowser);
}

void MemoryDeal::DelFreeOBJ(int nBrowser)
{
// 	char szOut[256] = { 0 };
// 	sprintf_s(szOut, 256, "DEL OBJ %d \n", nBrowser);
// 	OutputDebugStringA(szOut);

	for (std::vector<int>::iterator it = m_vBrowser.begin(); it != m_vBrowser.end(); it++)
	{
		if (*it == nBrowser)
		{
			m_vBrowser.erase(it);
			break;
		}
	}
}

bool MemoryDeal::HasFreeOBJ(int nBrowser)
{
// 	char szOut[256] = { 0 };
// 	sprintf_s(szOut, 256, "FIND OBJ %d \n", nBrowser);
// 	OutputDebugStringA(szOut);

	bool bFound = false;
	for (UINT i = 0; i < m_vBrowser.size(); i++)
	{
		if (m_vBrowser[i] == nBrowser)
		{
			bFound = true;
			break;
		}
	}
	return bFound;
}

long long GetTotalSpace(PSAPI_WORKING_SET_INFORMATION* wsi)
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	long long ret = 0;
	ULONG_PTR nPages = wsi->NumberOfEntries;

	for (DWORD i = 0; i <= nPages; i++)
	{
		if (wsi->WorkingSetInfo[i].Shared == 0)
		{
			ret++;
		}

	}
	return ret * si.dwPageSize;
}


void MemoryDeal::CheckThread()
{
	while (1)
	{
		DWORD dwRet = WaitForSingleObject(m_hExitEvent, m_dwCheckInterval);
		if (dwRet == WAIT_OBJECT_0) break;
		
		float fMemSize = 0;
		DWORD dwNow = GetTickCount();

		m_Lock.Lock();
		for (std::vector<stFreeTask>::iterator it = m_vTasks.begin() ; it != m_vTasks.end(); )
		{
			DWORD dwRet = WaitForSingleObject(it->hProcess, 0);
			if (dwRet == WAIT_OBJECT_0 || dwRet == WAIT_FAILED)
			{
				if (dwRet == WAIT_OBJECT_0) CloseHandle(it->hProcess);
				it = m_vTasks.erase(it);
			}
			else 
			{
				if (dwNow - it->dwLastFree > m_dwFreeInterval)
				{
					it->dwLastFree = dwNow;
					SetProcessWorkingSetSize(it->hProcess, -1, -1);
				}

				PSAPI_WORKING_SET_INFORMATION info, * wsi;

				PSAPI_WORKING_SET_INFORMATION* wsit = (PSAPI_WORKING_SET_INFORMATION*)malloc(sizeof(PSAPI_WORKING_SET_INFORMATION));

				QueryWorkingSet(it->hProcess, (LPVOID)&info, sizeof(&wsi));

				DWORD wsSize = sizeof(PSAPI_WORKING_SET_INFORMATION) + sizeof(PSAPI_WORKING_SET_BLOCK) * info.NumberOfEntries;

				wsi = (PSAPI_WORKING_SET_INFORMATION*)malloc(wsSize);

				if (QueryWorkingSet(it->hProcess, (LPVOID)wsi, wsSize)) 
				{
					long long totalSize = GetTotalSpace(wsi);
					float fPIDMemSize = totalSize*1.0f / 1024.0f / 1024.0f;
					fMemSize += fPIDMemSize;
				}
				free(wsi);
				it++;
			}
		}
		m_Lock.UnLock();
		m_fMemSize = fMemSize;
	}
}

void MemoryDeal::FreeMeme()
{
	float fMemSize = 0;
	DWORD dwNow = GetTickCount();

	m_Lock.Lock();
	for (std::vector<stFreeTask>::iterator it = m_vTasks.begin(); it != m_vTasks.end(); )
	{
		DWORD dwRet = WaitForSingleObject(it->hProcess, 0);
		if (dwRet == WAIT_OBJECT_0 || dwRet == WAIT_FAILED)
		{
			if (dwRet == WAIT_OBJECT_0) CloseHandle(it->hProcess);
			it = m_vTasks.erase(it);
		}
		else
		{
			it->dwLastFree = dwNow;
			SetProcessWorkingSetSize(it->hProcess, -1, -1);

			PSAPI_WORKING_SET_INFORMATION info, * wsi;

			PSAPI_WORKING_SET_INFORMATION* wsit = (PSAPI_WORKING_SET_INFORMATION*)malloc(sizeof(PSAPI_WORKING_SET_INFORMATION));

			QueryWorkingSet(it->hProcess, (LPVOID)&info, sizeof(&wsi));

			DWORD wsSize = sizeof(PSAPI_WORKING_SET_INFORMATION) + sizeof(PSAPI_WORKING_SET_BLOCK) * info.NumberOfEntries;

			wsi = (PSAPI_WORKING_SET_INFORMATION*)malloc(wsSize);

			if (QueryWorkingSet(it->hProcess, (LPVOID)wsi, wsSize))
			{
				long long totalSize = GetTotalSpace(wsi);
				float fPIDMemSize = totalSize * 1.0f / 1024.0f / 1024.0f;
				fMemSize += fPIDMemSize;
			}
			free(wsi);
			it++;
		}
	}
	m_Lock.UnLock();
	m_fMemSize = fMemSize;
}

float MemoryDeal::GetMemSize()
{
	return m_fMemSize;
}

