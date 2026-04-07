#pragma once
#include "windows.h"
#pragma comment(lib,"Shlwapi.lib")

class CIniParse 
{ 
public:
	CIniParse();
	CIniParse(char* szFileName,bool bAbsolut = false);
	CIniParse(wchar_t* szFileName, bool bAbsolut = false);

	void Open(char* szFileName, bool bAbsolut);
	void Open(wchar_t* szFileName, bool bAbsolut);

	virtual ~CIniParse();

	int GetInt(char* szSection, char *szName,int nDefault);
	BOOL SetInt(char *szSection, char *szName, int nValue);

	int GetInt(wchar_t* szSection, wchar_t *szName, int nDefault);
	BOOL SetInt(wchar_t *szSection, wchar_t *szName, int nValue);


	BOOL GetString(char* szSection, char *szName,char *szRet,int& nRetSize);
	BOOL SetString(char* szSection, char *szName,const char* szValue);

	BOOL GetWString(wchar_t* szSection, wchar_t *szName,wchar_t *szRet,int& nRetSize);
	BOOL SetWString(wchar_t* szSection, wchar_t *szName,const wchar_t* szValue);

private:
	char m_szFileName[MAX_PATH];
	wchar_t m_wszFileName[MAX_PATH];
};