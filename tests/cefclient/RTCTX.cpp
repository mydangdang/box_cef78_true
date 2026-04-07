#include "RTCTX.h"
#include <Shlwapi.h>
#include <IPHlpApi.h>
#include <winsock.h>
#include <WbemCli.h>
#include <comutil.h>
#include "string_util.h"

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib,"version.lib")
#pragma comment(lib, "wbemuuid.lib")


RTCTX::RTCTX()
{
	m_vOS.uOSVer = WIN_LAST;
}

RTCTX::~RTCTX()
{
}

typedef VOID(WINAPI* PFN_GetNativeSystemInfo)(OUT  LPSYSTEM_INFO);

bool RTCTX::IsX64()
{
	SYSTEM_INFO si = { 0 };

	PFN_GetNativeSystemInfo pfnGetNativeSystemInfo = \
		(PFN_GetNativeSystemInfo)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "GetNativeSystemInfo");

	if (pfnGetNativeSystemInfo)
	{
		pfnGetNativeSystemInfo(&si);

		if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
			si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
		{
			return TRUE;
		}
	}

	return FALSE;
}

std::string RTCTX::OSVer()
{
	if (!m_szOSVer.empty()) return m_szOSVer;

	std::string szVer;
	fnRtlGetVersion pRtlGetVersion;
	HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
	if (hNtdll == NULL) return 0;
	pRtlGetVersion = (fnRtlGetVersion)GetProcAddress(hNtdll, "RtlGetVersion");
	if (pRtlGetVersion == NULL) return 0;

	RTL_OSVERSIONINFOW VersionInformation = { 0 };
	VersionInformation.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
	LONG ntStatus = pRtlGetVersion(&VersionInformation);
	if (ntStatus != 0) return 0;

	m_vOS.uMajor = VersionInformation.dwMajorVersion;
	m_vOS.uMinor = VersionInformation.dwMinorVersion;
	m_vOS.uBuild = VersionInformation.dwBuildNumber;
	m_vOS.uOSVer = (UINT)MajorMinorBuildToVersion(m_vOS.uMajor, m_vOS.uMinor, m_vOS.uBuild);
	switch (m_vOS.uMajor)
	{
	case 5:
	{
		switch (m_vOS.uMinor)
		{
		case 0:
			m_szOSVer = "Win2000";
			break;
		case 1:
			m_szOSVer = "WinXP";
			break;
		case 2:
			m_szOSVer = "Win2003";
			break;
		default:
			m_szOSVer = "WinXP";
			break;
		}
	}
	break;
	case 6:
	{
		switch (m_vOS.uMinor)
		{
		case 0:
			m_szOSVer = "Win2008";
			break;
		case 1:
			m_szOSVer = "Win7";
			break;
		case 2:
			m_szOSVer = "Wind2012";
			break;
		case 3:
			m_szOSVer = "Win8";
			break;
		default:
			m_szOSVer = "Win7";
			break;
		}
	}
	break;
	case 10:
	{
		m_szOSVer = "Win10_";
		m_szOSVer += std::to_string(m_vOS.uBuild);
	}
	break;
	default:
		break;
	}
	return m_szOSVer;
}

std::string RTCTX::MAC()
{
	if (!m_szCurMAC.empty()) return m_szCurMAC;

	WORD wVersionRequested = MAKEWORD(2, 1);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);

	SOCKET Socket;
	sockaddr SockAddr;
	UINT32	IP;
	int	Len;
	std::map<UINT32, UINT64> IPMACList;
	UINT64 MAC = 0;

	GetIPMACList(IPMACList);
	do 
	{
		if (IPMACList.size() == 0) break;
		MAC = IPMACList.begin()->second;
		if (IPMACList.size() == 1) break;
		Socket = socket(AF_INET, SOCK_STREAM, 0);
		if (Socket == INVALID_SOCKET) break;
		ZeroMemory((void *)&SockAddr, sizeof(sockaddr));
		((sockaddr_in *)&SockAddr)->sin_family = AF_INET;
		((sockaddr_in *)&SockAddr)->sin_port = htons(80);
		IP = GetHostByName("www.baidu.com", 4000);
		if (!IP) break;
		((sockaddr_in *)&SockAddr)->sin_addr.s_addr = IP;
		if (::connect(Socket, &SockAddr, sizeof(sockaddr)) == SOCKET_ERROR)
		{
			closesocket(Socket);
			break;
		}
		Len = sizeof(sockaddr);
		getsockname(Socket, &SockAddr, &Len);
		IP = ((sockaddr_in *)&SockAddr)->sin_addr.s_addr;
		closesocket(Socket);
		MAC = IPMACList[IP];
	} while (0);

	UCHAR *bMacs = (UCHAR *)&MAC;
	char sMac[64] = { 0 };
	sprintf_s(sMac, 64, "%02X-%02X-%02X-%02X-%02X-%02X", bMacs[0], bMacs[1], bMacs[2], bMacs[3], bMacs[4], bMacs[5]);
	m_szCurMAC = sMac;
	WSACleanup();
	return m_szCurMAC;
}

std::string RTCTX::Version()
{
	if (!m_szCurVer.empty()) return m_szCurVer;

	char szVersion[32];
	TCHAR* pBuf = NULL;
	VS_FIXEDFILEINFO* pVsInfo = NULL;
	unsigned int iFileInfoSize = sizeof(VS_FIXEDFILEINFO);
	TCHAR szFileName[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, szFileName, MAX_PATH);
	int iVerInfoSize = GetFileVersionInfoSize(szFileName, NULL);
	if (iVerInfoSize != 0)
	{
		pBuf = new TCHAR[iVerInfoSize];
		if (GetFileVersionInfoW(szFileName, 0, iVerInfoSize, pBuf))
		{
			if (VerQueryValue(pBuf, L"\\", (void**)&pVsInfo, &iFileInfoSize))
			{
				sprintf_s(szVersion, "%d.%d.%d.%d",
					HIWORD(pVsInfo->dwFileVersionMS), LOWORD(pVsInfo->dwFileVersionMS),
					HIWORD(pVsInfo->dwFileVersionLS), LOWORD(pVsInfo->dwFileVersionLS));
			}
		}
		delete   pBuf;
		pBuf = NULL;
	}
	else
	{
		sprintf_s(szVersion, "%d.%d.%d.%d", 1, 0, 0, 0);
	}
	m_szCurVer = szVersion;
	return m_szCurVer;
}

std::string RTCTX::CurPath()
{
	if (!m_szCurPath.empty()) return m_szCurPath;

	char sProcessPath[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, sProcessPath, MAX_PATH);
	m_szCurExeName = PathFindFileNameA(sProcessPath);
	PathRemoveFileSpecA(sProcessPath);
	m_szCurPath = sProcessPath;
	return m_szCurPath;
}

std::string RTCTX::PCName()
{
	if (!m_szCurPCName.empty()) return m_szCurPCName;
	DWORD dwLen = 64;
	char sPcName[65] = { 0 };
	GetComputerNameA(sPcName, &dwLen);
	m_szCurPCName = sPcName;
	return m_szCurPCName;
}

std::string RTCTX::ExeName(bool bSubfix)
{
	if (!m_szCurExeName.empty())
	{
		if (!bSubfix)
		{
			int nPos = m_szCurExeName.find(".");
			std::string szRet = m_szCurExeName.substr(0, nPos);
			return szRet;
		}
		return m_szCurExeName;
	}
	char sProcessPath[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, sProcessPath, MAX_PATH);
	m_szCurExeName = PathFindFileNameA(sProcessPath);
	PathRemoveFileSpecA(sProcessPath);
	m_szCurPath = sProcessPath;

	if (!bSubfix)
	{
		int nPos = m_szCurExeName.find(".");
		std::string szRet = m_szCurExeName.substr(0, nPos);
		return szRet;
	}
	return m_szCurExeName;
}

std::string RTCTX::CPUID()
{
	INT32 dwBuf[4];
	std::string strCPUId;
	char buf[32] = { 0 };
	__cpuidex(dwBuf, 1, 1);
	memset(buf, 0, 32);
	sprintf_s(buf, 32, "%08X", dwBuf[3]);
	strCPUId += buf;
	memset(buf, 0, 32);
	sprintf_s(buf, 32, "%08X", dwBuf[0]);
	strCPUId += buf;
	return strCPUId;
}

emOsVersion RTCTX::MajorMinorBuildToVersion(int major, int minor, int build)
{
	if (major == 10) {
		if (build >= 18363)
			return WIN10_19H2;
		if (build >= 18362)
			return WIN10_19H1;
		if (build >= 17763)
			return WIN10_RS5;
		if (build >= 17134)
			return WIN10_RS4;
		if (build >= 16299)
			return WIN10_RS3;
		if (build >= 15063)
			return WIN10_RS2;
		if (build >= 14393)
			return WIN10_RS1;
		if (build >= 10586)
			return WIN10_TH2;
		return WIN10;
	}

	if (major > 6) {
		return WIN_LAST;
	}

	if (major == 6) {
		switch (minor) {
		case 0:
			return VISTA;
		case 1:
			return WIN7;
		case 2:
			return WIN8;
		default:
			return WIN8_1;
		}
	}

	if (major == 5 && minor != 0) {
		return minor == 1 ? XP : SERVER_2003;
	}

	return PRE_XP;
}

LSTATUS RTCTX::GetIPMACList(std::map<UINT32, UINT64>& IP_MACMapList)
{
	PIP_ADAPTER_INFO		pAdapterInfo = NULL;
	PIP_ADAPTER_INFO		pAdapter = NULL;
	ULONG					ulOutBufLen;
	UINT32					IP;
	UINT64					MAC;
	LSTATUS					lStatus;

	do
	{
		ulOutBufLen = sizeof(IP_ADAPTER_INFO);
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
		if (pAdapterInfo == NULL)	break;

		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
		{
			free(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
			if (pAdapterInfo == NULL) break;
		}
		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) != NO_ERROR) break;

		pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			MAC = *(UINT64*)pAdapter->Address;
			IP = inet_addr(pAdapter->IpAddressList.IpAddress.String);
			if (IP != 0) IP_MACMapList[IP] = MAC;
			pAdapter = pAdapter->Next;
		}
	} while (FALSE);
	lStatus = GetLastError();
	if (pAdapterInfo) free(pAdapterInfo);
	return lStatus;
}

UINT32 RTCTX::GetHostByName(const char* sHost, UINT32 dwTimeOut )
{
	char                    *P;
	struct hostent          *pHostent;
	UINT32					Tick = GetTickCount();
	LSTATUS					lStatus;
	while (GetTickCount() - Tick < dwTimeOut)
	{
		pHostent = gethostbyname(sHost);
		if (pHostent != NULL)
		{
			P = pHostent->h_addr_list[0];
			return *((UINT32 *)P);
		}
		lStatus = GetLastError();
		Sleep(100);
		SetLastError(lStatus);
	}
	return 0;
}

void Trims(char* data)
{
	int i = -1, j = 0;
	int ch = ' ';

	while (data[++i] != '\0')
	{
		if (data[i] != ch)
		{
			data[j++] = data[i];
		}
	}
	data[j] = '\0';
}

std::string RTCTX::SysDiskID()
{
	char sysDiskID[256] = { 0 };
	HRESULT hres;
	IWbemLocator *pLoc = NULL;
	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID *)&pLoc);
	if (FAILED(hres))
	{
		return "";
	}
	IWbemServices *pSvc = NULL;
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"),
		NULL,
		NULL,
		0,
		NULL,
		0,
		0,
		&pSvc
	);
	if (FAILED(hres))
	{
		pLoc->Release();
		return "";
	}
	hres = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
	);
	if (FAILED(hres))
	{
		pSvc->Release();
		pLoc->Release();
		return "";
	}

	IEnumWbemClassObject* pEnumerator = NULL;
	IWbemClassObject *pclsObj;
	ULONG uReturn = 0;
	int diskIndex = 0;
	pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t(L"WQL"),
		bstr_t(L"SELECT * FROM Win32_DiskPartition WHERE Bootable = TRUE"),  //²éÕÒÆô¶¯ÅÌ
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);
	if (FAILED(hres))
	{
		pSvc->Release();
		pLoc->Release();
		return "";
	}
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
		if (0 == uReturn)
		{
			break;
		}
		VARIANT vtProp;
		hr = pclsObj->Get(L"DiskIndex", 0, &vtProp, 0, 0);
		diskIndex = vtProp.intVal;
		VariantClear(&vtProp);
		pclsObj->Release();
	}

	char index[10];
	std::string strQuery = "SELECT * FROM Win32_DiskDrive WHERE Index = ";
	itoa(diskIndex, index, 10);
	std::string indexStr(index);
	strQuery += indexStr;
	pEnumerator->Release();
	pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t(strQuery.c_str()),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);
	if (FAILED(hres))
	{
		pSvc->Release();
		pLoc->Release();
		return "";
	}
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
		if (0 == uReturn)
		{
			break;
		}
		VARIANT vtProp;
		VariantInit(&vtProp);
		VariantClear(&vtProp);
		hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
		if(vtProp.vt == VT_BSTR && vtProp.bstrVal != NULL)
		{
			wcstombs(sysDiskID, vtProp.bstrVal, 20);
			Trims(sysDiskID);
		}
		VariantClear(&vtProp);
		pclsObj->Release();
	}

	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();

	return sysDiskID;
}



stWinVer RTCTX::m_vOS;

std::string RTCTX::m_szOSVer;

std::string RTCTX::m_szCurMAC;

std::string RTCTX::m_szCurVer;

std::string RTCTX::m_szCurPath;

std::string RTCTX::m_szCurPCName;

std::string RTCTX::m_szCurExeName;
