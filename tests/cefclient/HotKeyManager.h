#pragma once
#include <string>
#include <wtypes.h>
#include <vector>
#include <thread>

#define HK_BOSS "bosskey"
#define HK_TOPFULL "topfull"
#define HK_RECORD "record"
#define HK_REPLAY "replay"
#define HK_KEEPCLICK "keepclick"
#define HK_SNAPSHOT "snapshot"
#define HK_STOPRECORDSCREEN "stoprecordscreen"

struct stHotKey
{
	UINT uID;
	UINT uKey;
	UINT uModifier1;
	UINT uModifier2;
	std::string szName;
};

class HotKeyObserver
{
public:
	virtual void OnHotKey(stHotKey* pHotKey) = 0;
};

class HotKeyManager
{
public:
	HotKeyManager();
	~HotKeyManager();

	bool AddHotKey(std::string szName, UINT uKey, UINT uModifier1 = 0, UINT uModifier2 = 0, BOOL bReplace = FALSE,BOOL bInit = FALSE);
	bool DelHotKey(std::string szName);
	void MonitHotKey();
	stHotKey* GetHotKey(std::string szName);

	UINT GetModifier(UINT vkCode);
	void RegisteHotKey(stHotKey* pHotKey,bool bRegist = true);
	void UpdateToLocalStorage();
	void ReadFromLocalStorage();

	int m_nSlefAddID;
	bool m_bQuit;
	std::vector<stHotKey*> m_vHotKeys;
	std::thread m_tHotKeyMonitor;
	HotKeyObserver* m_pObserver;
};

extern HotKeyManager hotKeyManager;
