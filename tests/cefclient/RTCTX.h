#pragma once
#include <map>
#include <string>
#include <windows.h>

enum emOsVersion
{
	PRE_XP = 0,  // Not supported.
	XP = 1,
	SERVER_2003 = 2,  // Also includes XP Pro x64 and Server 2003 R2.
	VISTA = 3,        // Also includes Windows Server 2008.
	WIN7 = 4,         // Also includes Windows Server 2008 R2.
	WIN8 = 5,         // Also includes Windows Server 2012.
	WIN8_1 = 6,       // Also includes Windows Server 2012 R2.
	WIN10 = 7,        // Threshold 1: Version 1507, Build 10240.
	WIN10_TH2 = 8,    // Threshold 2: Version 1511, Build 10586.
	WIN10_RS1 = 9,    // Redstone 1: Version 1607, Build 14393.
	WIN10_RS2 = 10,   // Redstone 2: Version 1703, Build 15063.
	WIN10_RS3 = 11,   // Redstone 3: Version 1709, Build 16299.
	WIN10_RS4 = 12,   // Redstone 4: Version 1803, Build 17134.
	WIN10_RS5 = 13,   // Redstone 5: Version 1809, Build 17763.
	WIN10_19H1 = 14,  // 19H1: Version 1903, Build 18362.
	WIN10_19H2 = 15,  // 19H2: Version 1909, Build 18363
	WIN_LAST,  // Indicates error condition.
};

struct stWinVer
{
	UINT uOSVer;
	UINT uMajor;
	UINT uMinor;
	UINT uBuild;
};

class RTCTX
{
	typedef LONG(__stdcall *fnRtlGetVersion)(PRTL_OSVERSIONINFOW lpVersionInformation);
public:
	RTCTX();
	~RTCTX();

	static bool IsX64();
	static std::string MAC();
	static std::string OSVer();
	static std::string Version();
	static std::string CurPath();
	static std::string PCName();
	static std::string ExeName(bool bSubfix = true);
	static std::string CPUID();
	static std::string SysDiskID();

	static emOsVersion MajorMinorBuildToVersion(int major, int minor, int build);
protected:
	static LSTATUS GetIPMACList(std::map<UINT32, UINT64>& IP_MACMapList);
	static UINT32 GetHostByName(const char* sHost, UINT32 dwTimeOut = 2000);
	
protected:
	static stWinVer m_vOS;
	static std::string m_szOSVer;
	static std::string m_szCurMAC;
	static std::string m_szCurVer;
	static std::string m_szCurPath;
	static std::string m_szCurPCName;
	static std::string m_szCurExeName;
};