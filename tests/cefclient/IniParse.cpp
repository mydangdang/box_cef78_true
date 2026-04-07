#include "IniParse.h" 
#include <Shlwapi.h>
#include <string>


wchar_t* AnsiToUnicode(char *str)
{
	DWORD dwNum = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	wchar_t *pwText = NULL;
	pwText = new wchar_t[dwNum];
	if (!pwText) return NULL;
	MultiByteToWideChar(CP_ACP, 0, str, -1, pwText, dwNum);
	return pwText;
}

char * UnicodeToAnsi(wchar_t *wText)
{
	DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);
	char *psText;
	psText = new char[dwNum];
	if (!psText) return NULL;
	WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, psText, dwNum, NULL, FALSE);
	return psText;
}

CIniParse::CIniParse(char* szFileName, bool bAbsolut)
{
	Open(szFileName, bAbsolut);
}

CIniParse::CIniParse(wchar_t* szFileName, bool bAbsolut)
{
	Open(szFileName, bAbsolut);
}

CIniParse::CIniParse()
{

}

void CIniParse::Open(char* szFileName, bool bAbsolut)
{
	std::string strFilePath = szFileName;
	if(!bAbsolut)
	{
		strFilePath.clear();
		CHAR szFilePath[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
		PathRemoveFileSpecA(szFilePath);
		strFilePath = szFilePath;
		strFilePath += "\\";
		strFilePath += std::string(szFileName);
	}
	HANDLE hFile = CreateFileA(strFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

	memset(m_szFileName, 0, MAX_PATH);
	strcpy_s(m_szFileName, MAX_PATH, strFilePath.c_str());
	wchar_t *wszFileName = AnsiToUnicode(m_szFileName);
	if (wszFileName)
	{
		wcscpy_s(m_wszFileName, MAX_PATH, wszFileName);
		delete wszFileName;
		
	}
}

void CIniParse::Open(wchar_t* szFileName, bool bAbsolut)
{
	std::wstring strFilePath = szFileName;
	if(!bAbsolut)
	{
		strFilePath.clear();
		TCHAR szFilePath[MAX_PATH] = { 0 };
		GetModuleFileName(NULL, szFilePath, MAX_PATH);
		PathRemoveFileSpec(szFilePath);
		strFilePath = szFilePath;
		strFilePath += L"\\";
		strFilePath += std::wstring(szFileName);
	}
	HANDLE hFile = CreateFile(strFilePath.c_str(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

	memset(m_wszFileName, 0, MAX_PATH);
	wcscpy_s(m_wszFileName, MAX_PATH, strFilePath.c_str());
	char *szName = UnicodeToAnsi(m_wszFileName);
	strcpy_s(m_szFileName, MAX_PATH, szName);
	delete szName;

}

CIniParse::~CIniParse() 
{

}

int CIniParse::GetInt(char* szSection, char *szName, int nDefault)
{
	char szBuffer[256] = { 0 };
	int nCopied = GetPrivateProfileStringA(szSection, szName,"",szBuffer, 256,m_szFileName);
	if(!nCopied) return nDefault;
	else
	{
		nDefault = atoi(szBuffer);
		return nDefault;
	}
}

int CIniParse::GetInt(wchar_t* szSection, wchar_t *szName, int nDefault)
{
	wchar_t szBuffer[256] = { 0 };
	int nCopied = GetPrivateProfileString(szSection, szName, L"", szBuffer, 256, m_wszFileName);
	if (!nCopied) return nDefault;
	else
	{
		nDefault = _wtoi(szBuffer);
		return nDefault;
	}
}

BOOL CIniParse::SetInt(char *szSection, char *szName, int nValue)
{
	char szInput[256] = { 0 };
	sprintf_s(szInput, "%d", nValue);
	return WritePrivateProfileStringA(szSection, szName, szInput,m_szFileName);
}


BOOL CIniParse::SetInt(wchar_t *szSection, wchar_t *szName, int nValue)
{
	wchar_t szInput[256] = { 0 };
	swprintf_s(szInput, L"%d", nValue);
	return WritePrivateProfileString(szSection, szName, szInput, m_wszFileName);
}

BOOL CIniParse::GetString(char* szSection, char *szName, char *szRet, int& nRetSize)
{
	if (!szRet)
	{
		nRetSize = 0;
		return FALSE;
	}
	int nCopied = GetPrivateProfileStringA(szSection, szName, "", szRet, nRetSize, m_szFileName);
	if (!nCopied)
	{
		nRetSize = nCopied;
		return FALSE;
	}
	else
	{
		nRetSize = nCopied;
		return TRUE;
	}
}

BOOL CIniParse::GetWString(wchar_t* szSection, wchar_t *szName, wchar_t *szRet, int& nRetSize)
{
	if (!szRet)
	{
		nRetSize = 0;
		return FALSE;
	}
	int nCopied = GetPrivateProfileString(szSection, szName, L"", szRet, nRetSize, m_wszFileName);
	if (!nCopied)
	{
		nRetSize = nCopied;
		return FALSE;
	}
	else
	{
		nRetSize = nCopied;
		return TRUE;
	}
}

BOOL CIniParse::SetString(char* szSection, char *szName, const char* szValue)
{
	return WritePrivateProfileStringA(szSection, szName, szValue, m_szFileName);
}

BOOL CIniParse::SetWString(wchar_t* szSection, wchar_t *szName,const wchar_t* szValue)
{
	return WritePrivateProfileString(szSection, szName, szValue, m_wszFileName);
}