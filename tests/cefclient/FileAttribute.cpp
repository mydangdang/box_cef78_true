#include "FileAttribute.h"  
#pragma comment(lib, "version")

namespace UtilFileAttribute
{
	bool QueryValue(const std::string& ValueName, const std::string& szModuleName, std::string& RetStr)
	{
		bool bSuccess = FALSE;
		BYTE* m_lpVersionData = NULL;
		DWORD   m_dwLangCharset = 0;
		CHAR* tmpstr = NULL;

		do
		{
			if (!ValueName.size() || !szModuleName.size())
				break;

			DWORD dwHandle;
			DWORD dwDataSize = ::GetFileVersionInfoSizeA((LPCSTR)szModuleName.c_str(), &dwHandle);
			if (dwDataSize == 0)
				break;

			m_lpVersionData = new (std::nothrow) BYTE[dwDataSize];
			if (NULL == m_lpVersionData)
				break;

			if (!::GetFileVersionInfoA((LPCSTR)szModuleName.c_str(), dwHandle, dwDataSize,
				(void*)m_lpVersionData))
				break;

			UINT nQuerySize;
			DWORD* pTransTable;
			if (!::VerQueryValueA(m_lpVersionData, "\\VarFileInfo\\Translation", (void**)&pTransTable, &nQuerySize))
				break;

			m_dwLangCharset = MAKELONG(HIWORD(pTransTable[0]), LOWORD(pTransTable[0]));
			if (m_lpVersionData == NULL)
				break;

			tmpstr = new (std::nothrow) CHAR[128];
			if (NULL == tmpstr)
				break;
			sprintf_s(tmpstr, 128, "\\StringFileInfo\\%08lx\\%s", m_dwLangCharset, ValueName.c_str());
			LPVOID lpData;

			if (::VerQueryValueA((void*)m_lpVersionData, tmpstr, &lpData, &nQuerySize))
				RetStr = (char*)lpData;

			bSuccess = TRUE;
		} while (FALSE);

		if (m_lpVersionData)
		{
			delete[] m_lpVersionData;
			m_lpVersionData = NULL;
		}
		if (tmpstr)
		{
			delete[] tmpstr;
			tmpstr = NULL;
		}

		return bSuccess;
	}

	bool	GetFileDescription(const std::string& szModuleName, std::string& RetStr) { return QueryValue("FileDescription", szModuleName, RetStr); };
	bool	GetFileVersion(const std::string& szModuleName, std::string& RetStr) { return QueryValue("FileVersion", szModuleName, RetStr); };
	bool	GetInternalName(const std::string& szModuleName, std::string& RetStr) { return QueryValue("InternalName", szModuleName, RetStr); };
	bool	GetCompanyName(const std::string& szModuleName, std::string& RetStr) { return QueryValue("CompanyName", szModuleName, RetStr); };
	bool	GetLegalCopyright(const std::string& szModuleName, std::string& RetStr) { return QueryValue("LegalCopyright", szModuleName, RetStr); };
	bool	GetOriginalFilename(const std::string& szModuleName, std::string& RetStr) { return QueryValue("OriginalFilename", szModuleName, RetStr); };
	bool	GetProductName(const std::string& szModuleName, std::string& RetStr) { return QueryValue("ProductName", szModuleName, RetStr); };
	bool	GetProductVersion(const std::string& szModuleName, std::string& RetStr) {return QueryValue("ProductVersion", szModuleName, RetStr);};
}