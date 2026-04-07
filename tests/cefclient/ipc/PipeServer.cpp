#include "PipeServer.h"
#include "sstream"
#include "Random.h"

#define BUFF_SIZE 4096
IPipeServerListener::IPipeServerListener()
{
	m_bExitMonitor = false;
	m_hRequestEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hRequestMonitor = (HANDLE)_beginthreadex(NULL, 0,(unsigned (_stdcall *) (void *))ThreadRequestMonitor, (void *)this, 0, NULL);
}

IPipeServerListener::~IPipeServerListener()
{
	TerminateThreadRequestMonitor();
}

void IPipeServerListener::OnServerStart()
{

}

void IPipeServerListener::OnServerStop()
{

}

std::string IPipeServerListener::OnSyncRequest( string strRequest)
{
	return "";
}

void IPipeServerListener::OnAsyncRequest( string strRequest )
{

}

void IPipeServerListener::PushAsyncRequest( string strRequest )
{
	m_vRequests.PushBuffer(strRequest);
	SetEvent(m_hRequestEvent);
}

DWORD WINAPI IPipeServerListener::ThreadRequestMonitor( LPVOID lParam )
{
	if(!lParam) return 0;
	IPipeServerListener *pListener = (IPipeServerListener *)lParam;
	while(!pListener->m_bExitMonitor)
	{
		WaitForSingleObject(pListener->m_hRequestEvent,-1);
		string strRequest;
		BufferReadRES res = pListener->m_vRequests.PopBuffer(strRequest);
		if(res == BUFFER_POPOK)
			pListener->OnAsyncRequest(strRequest);
		if(res != BUFFER_EMPTY)
			::SetEvent(pListener->m_hRequestEvent);
	}
	return 0;
}

void IPipeServerListener::TerminateThreadRequestMonitor()
{
	m_bExitMonitor = true;
	if(m_hRequestEvent) SetEvent(m_hRequestEvent);
	if(m_hRequestMonitor)
	{
		::WaitForSingleObject(m_hRequestMonitor,-1);
		::CloseHandle(m_hRequestMonitor);
		m_hRequestMonitor = NULL;
	}
	if(m_hRequestEvent)
	{
		CloseHandle(m_hRequestEvent);
		m_hRequestEvent = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////

CPipeServer::CPipeServer(IPipeServerListener *pListener)
{
    m_bExit = false;
    int nSectret = 0;
	RandomSeed();
    do
    {
		nSectret = RandomGen();;
    } while (nSectret <= 0);


    int nProcessID = ::GetCurrentProcessId();
	stringstream ss;
	ss << "\\\\.\\pipe\\" << nProcessID << "_" << nSectret;
	m_strPipeName = ss.str();
    m_pListener = pListener;
    m_hPipeServrThread = NULL;
}

CPipeServer::~CPipeServer(void)
{
	TerminateThreadPipeServer();
}

void CPipeServer::InitPipeServer(std::string szName)
{
	if (!szName.empty()) m_strPipeName = szName;
	if(!m_hPipeServrThread)
	{
		m_hPipeServrThread = (HANDLE)_beginthreadex(NULL, 0,(unsigned (_stdcall *) (void *))ThreadPipeServer, (void *)this, 0, NULL);
		if(m_hPipeServrThread&&m_pListener) m_pListener->OnServerStart();

	}
}

DWORD WINAPI CPipeServer::ThreadPipeServer( LPVOID lParam )
{
	if(!lParam) return 0;
	DWORD dwErrorCode = 0;
	
	CPipeServer *pServer = (CPipeServer *)lParam;
	HANDLE hPipe = CreateNamedPipeA(pServer->GetPipeSrvName().c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BUFF_SIZE, BUFF_SIZE, 0, NULL);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		dwErrorCode = GetLastError();
		return 0;
	}

	while(!pServer->IsExit())
	{
		BOOL bRes = ConnectNamedPipe(hPipe, NULL) ? TRUE : (dwErrorCode = GetLastError() == ERROR_PIPE_CONNECTED); 
		if(bRes) 
		{
			DWORD dwReaded = 0;
			char tmpBuffer[BUFF_SIZE] = {0};
		
			DWORD dwErrorCode = 0;
			string strRes = "";
			do 
			{
				dwReaded = 0;
				memset(tmpBuffer,0,BUFF_SIZE);
				if(!ReadFile(hPipe,tmpBuffer,BUFF_SIZE,&dwReaded,NULL))
				{
					dwErrorCode = GetLastError();
					if(dwErrorCode == ERROR_MORE_DATA && dwReaded)
						strRes.append(tmpBuffer,dwReaded);
					else break;
				}
				else
				{
					if(dwReaded) strRes.append(tmpBuffer,dwReaded);
					break;
				}
			} while(1);
			
			if(!strRes.empty())
			{
				char flag = strRes.at(0);
				strRes = strRes.substr(1);
				if(flag == '0') pServer->SyncRequest(hPipe, strRes);
				else if (flag == '1') pServer->AsyncRequest(hPipe, strRes);
			}
		} 
	}
	CloseHandle(hPipe);
	return 0;
}

void CPipeServer::SyncRequest(HANDLE hPipe, string strRes)
{
	if(m_pListener) 
	{

		std::string strBuffer = m_pListener->OnSyncRequest(strRes);
	
		DWORD dwWrited = 0;
		WriteFile(hPipe, strBuffer.c_str(), strBuffer.size(), &dwWrited, NULL);
	}

	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe);
	return;
}

void CPipeServer::AsyncRequest(HANDLE hPipe, string strRes)
{
	PushAsyncRequest(strRes);
	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe); 
}

void CPipeServer::TerminateThreadPipeServer()
{
	m_bExit = true;
	if(m_hPipeServrThread)
	{
		DeleteFileA(m_strPipeName.c_str());
		::WaitForSingleObject(m_hPipeServrThread,-1);
		::CloseHandle(m_hPipeServrThread);
		m_hPipeServrThread = NULL;
		if(m_pListener) m_pListener->OnServerStop();
	}
	m_pListener = NULL;
}

void CPipeServer::PushAsyncRequest(string strRequest)
{
	if(strRequest.empty()) return;
	if(m_pListener) m_pListener->PushAsyncRequest(strRequest);
}


bool CPipeServer::IsExit()
{
	return m_bExit;
}

string CPipeServer::GetPipeSrvName()
{
	return m_strPipeName;
}