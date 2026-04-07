#pragma once
#include "DoubleBufferQueue.h"
#include <string>
#include <vector>
#include "atlbase.h"
using namespace std;

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) if(x){delete x;x=NULL;} 
#endif


class CPipeServer;
typedef struct AsyncRequestParam 
{
	CPipeServer *pServer;
	HANDLE hPipe;
}* PAsyncRequestParam;

typedef vector<HANDLE> vAsyncPipes;


class IPipeServerListener
{
public:
	IPipeServerListener();
	virtual ~IPipeServerListener();
	virtual void OnServerStart();
	virtual void OnServerStop();
	virtual std::string OnSyncRequest(string strRequest);
	virtual void OnAsyncRequest(string strRequest);

	void PushAsyncRequest(string strRequest);
protected:
	static DWORD WINAPI ThreadRequestMonitor(LPVOID lParam);
	void TerminateThreadRequestMonitor();
public:
	bool m_bExitMonitor;
	CDoubleBufferQueue<string> m_vRequests;
protected:
	HANDLE m_hRequestMonitor;
	HANDLE m_hRequestEvent;;
};

class CPipeServer
{
public:
    CPipeServer(IPipeServerListener *pListener);
	~CPipeServer(void);
	void InitPipeServer(std::string szName);
	bool IsExit();
	string GetPipeSrvName();
protected:
	static DWORD WINAPI ThreadPipeServer(LPVOID lParam);
	void TerminateThreadPipeServer();

	void SyncRequest(HANDLE hPipe,string strRes);
	void AsyncRequest(HANDLE hPipe,string strRes);

	void PushAsyncRequest(string strRequest);
protected:
	bool m_bExit;

	string m_strPipeName;

	vAsyncPipes m_vAsyncPipes;
	
	
	HANDLE m_hPipeServrThread;
	IPipeServerListener *m_pListener;
};

