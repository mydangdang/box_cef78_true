#include "PipeClient.h"

#define BUFF_SIZE 4096
CPipeClient::CPipeClient(string strPipeName)
{
	m_strPipeName = strPipeName;
}


CPipeClient::~CPipeClient(void)
{
}

std::string CPipeClient::SendRequest(string szJsonCmd, int nTimeOut)
{
	if(szJsonCmd.empty()) return "";
	DWORD dwErrorCode;
	HANDLE hPipe = ConnectToServer(nTimeOut);
	if(!hPipe) return "";
	int nSize = szJsonCmd.length()+2;
	char *buffer = new char[nSize];
	memset(buffer, 0, nSize);
	buffer[0] = '0';
	sprintf_s(buffer+1,nSize-1,"%s",szJsonCmd.c_str());
	buffer[nSize-1] = '\0';
	DWORD dwWrited = 0;
	BOOL bRes = WriteFile(hPipe,buffer,nSize,&dwWrited,NULL);
	if (!bRes) 
	{
		dwErrorCode = GetLastError();
		delete[] buffer;
		buffer = NULL;
		CloseHandle(hPipe);
		return "";
	}
	else if(dwWrited != nSize)
	{

		delete[] buffer;
		buffer = NULL;
		CloseHandle(hPipe);
		return "";
	}
	DWORD dwReaded = 0;
	char tmpBuffer[BUFF_SIZE] = {0};
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

	delete[] buffer;
	buffer = NULL;
	CloseHandle(hPipe);
	return strRes;
}

void CPipeClient::PostReuest(string szJsonCmd)
{
	if (szJsonCmd.empty()) return;
	HANDLE hPipe = ConnectToServer(-1);
	if(!hPipe) return;
	int nSize = szJsonCmd.length()+2;
	char *buffer = new char[nSize];
	memset(buffer, 0, nSize);
	buffer[0] = '1';
	sprintf_s(buffer+1,nSize-1,szJsonCmd.c_str());
	buffer[nSize-1] = '\0';
	DWORD dwWrited = 0;
	WriteFile(hPipe,buffer,nSize,&dwWrited,NULL);
	delete[] buffer;
	buffer = NULL;
	CloseHandle(hPipe); 
}

HANDLE CPipeClient::ConnectToServer( int nTimeOut )
{
	HANDLE hPipe = NULL;
	while (1) 
	{
		hPipe = CreateFileA(m_strPipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hPipe != INVALID_HANDLE_VALUE) break; 
		DWORD dwErrorCode = GetLastError();
		
		if (dwErrorCode != ERROR_PIPE_BUSY)
		{
			return NULL;
		}

		if (!WaitNamedPipeA(m_strPipeName.c_str(), nTimeOut)) 
		{
			dwErrorCode = GetLastError();
			return NULL;
		} 
	}
	return hPipe;
}