#pragma once
#include "windows.h"
#include <string>
using namespace std;

class CPipeClient
{
public:
	CPipeClient(string PipeName);
	~CPipeClient(void);
	
	string SendRequest(string szJsonCmd,int nTimeOut = -1);
	void PostReuest(string szJsonCmd);
protected:
	HANDLE ConnectToServer(int nTimeOut);
protected:
	string m_strPipeName;
};

