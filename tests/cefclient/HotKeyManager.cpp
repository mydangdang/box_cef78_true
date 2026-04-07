#include "HotKeyManager.h"
#include "UtilLocalStorage.h"
#include <string>
#include "include/cef_parser.h"

#define WM_REG_HOTKEY WM_USER+1001

HotKeyManager hotKeyManager;

HotKeyManager::HotKeyManager()
{
	m_nSlefAddID = 1000;
	m_pObserver = NULL;
	m_bQuit = false;
	m_tHotKeyMonitor = std::thread(&HotKeyManager::MonitHotKey, this);
}

HotKeyManager::~HotKeyManager()
{
	m_bQuit = true;
	if (m_tHotKeyMonitor.joinable())
	{
		PostThreadMessage(GetThreadId(m_tHotKeyMonitor.native_handle()), WM_QUIT, 0, 0);
		m_tHotKeyMonitor.join();
	}

	auto pHotKeyIt = m_vHotKeys.begin();
	for (; pHotKeyIt != m_vHotKeys.end(); pHotKeyIt++)
	{
		stHotKey* pHotKey = *pHotKeyIt;
		delete pHotKey;
	}
	m_vHotKeys.clear();
}

UINT HotKeyManager::GetModifier(UINT vkCode) 
{
	switch (vkCode)
	{
	case VK_CONTROL:
		return MOD_CONTROL;
	case VK_SHIFT:
		return MOD_SHIFT;
	case VK_MENU:
		return MOD_ALT;
	default:
		break;
	}
	return 0;
}

void HotKeyManager::MonitHotKey()
{
	MSG msg;
	while (!m_bQuit &&  GetMessage(&msg, nullptr, 0, 0))
	{
		if (msg.message == WM_HOTKEY)
		{
			//������Ȼ���߳���, �������Ⱥ��ϵ,�������ٳ���,���Բ�������.
			if (m_pObserver)
			{
				for each (auto pHotKey in m_vHotKeys)
				{
					if (pHotKey->uID == msg.wParam) m_pObserver->OnHotKey(pHotKey);
				}
			}
		}
		else if (msg.message == WM_REG_HOTKEY)
		{
			stHotKey* pHotKey = (stHotKey*)msg.wParam;

			UnregisterHotKey(NULL, pHotKey->uID);
			if (msg.lParam)
			{
				UINT uModifier = GetModifier(pHotKey->uModifier1);
				uModifier |= GetModifier(pHotKey->uModifier2);
				RegisterHotKey(NULL, pHotKey->uID, uModifier, pHotKey->uKey);
			}
		}
		else if (msg.message == WM_QUIT) break;
	}
}

stHotKey* HotKeyManager::GetHotKey(std::string szName)
{
	stHotKey* pResHotKey = NULL;
	bool bFound = false;
	for each (auto pHotKey in m_vHotKeys)
	{
		if (pHotKey->szName == szName)
		{
			pResHotKey = pHotKey;
			bFound = true;
			break;
		}
	}
	return pResHotKey;
}

bool HotKeyManager::AddHotKey(std::string szName, UINT uKey, UINT uModifier1, UINT uModifier2, BOOL bReplace, BOOL bInit)
{
	bool bFound = false;
	for each (auto pHotKey in m_vHotKeys)
	{
		if (pHotKey->szName == szName)
		{
			bFound = true;
			if (!bReplace) return false;
			else if (uKey != pHotKey->uKey || uModifier1 != pHotKey->uModifier1 || uModifier2 != pHotKey->uModifier2)
			{
				RegisteHotKey(pHotKey, false);
				pHotKey->uKey = uKey;
				pHotKey->uModifier1 = uModifier1;
				pHotKey->uModifier2 = uModifier2;
				RegisteHotKey(pHotKey, true);
				if (bFound && !bInit) UpdateToLocalStorage();
			}
			break;
		}
	}
	if (!bFound)
	{
		stHotKey* pHotKey = new stHotKey;
		pHotKey->szName = szName;
		pHotKey->uID = m_nSlefAddID++;
		pHotKey->uKey = uKey;
		pHotKey->uModifier1 = uModifier1;
		pHotKey->uModifier2 = uModifier2;
		RegisteHotKey(pHotKey, true);
		m_vHotKeys.push_back(pHotKey);
		if (!bInit) UpdateToLocalStorage();
	}
	return true;
}

bool HotKeyManager::DelHotKey(std::string szName)
{
	bool bFound = false;
	auto pHotKey = m_vHotKeys.begin();
	for (; pHotKey != m_vHotKeys.end(); pHotKey++)
	{
		if ((*pHotKey)->szName == szName)
		{
			bFound = true;
			RegisteHotKey((*pHotKey), false);
			m_vHotKeys.erase(pHotKey);
			break;
		}
	}
	if (bFound) UpdateToLocalStorage();
	return bFound;
}

void HotKeyManager::RegisteHotKey(stHotKey* pHotKey, bool bRegist)
{
	PostThreadMessage(GetThreadId(m_tHotKeyMonitor.native_handle()), WM_REG_HOTKEY, (WPARAM)pHotKey, (LPARAM)bRegist);
}

void HotKeyManager::UpdateToLocalStorage()
{
	CefRefPtr<CefListValue> hotKeyList = CefListValue::Create();

	int nIdx = 0;
	for each (auto pHotKey in m_vHotKeys)
	{
		CefRefPtr<CefDictionaryValue> itemKey = CefDictionaryValue::Create();
		itemKey->SetString("name", pHotKey->szName);
		itemKey->SetInt("modifier1", pHotKey->uModifier1);
		itemKey->SetInt("modifier2", pHotKey->uModifier2);
		itemKey->SetInt("key", pHotKey->uKey);
		hotKeyList->SetDictionary(nIdx++,itemKey);
	}

	CefRefPtr<CefValue> value = CefValue::Create();
	value->SetList(hotKeyList);
	CefString szJson = CefWriteJSON(value, JSON_WRITER_DEFAULT);

	LSHelper.UpdateLocalStorage(L"hotkeys", szJson);
}

void HotKeyManager::ReadFromLocalStorage()
{
	std::wstring szHotKeys = LSHelper.GetLocalStorage(L"hotkeys");

	if (szHotKeys.empty()) return;
	CefRefPtr<CefValue> value = CefParseJSON(szHotKeys, JSON_PARSER_RFC);
	if (!value) return ;
	CefRefPtr<CefListValue> hotKeyList = value->GetList();
	
	for(UINT i = 0 ;i < hotKeyList->GetSize() ; i++)
	{
		CefRefPtr<CefDictionaryValue> itemKey = hotKeyList->GetDictionary(i);
		if (itemKey)
		{
			std::string szName = itemKey->GetString("name");
			UINT uModifier1 = itemKey->GetInt("modifier1");
			UINT uModifier2 = itemKey->GetInt("modifier2");
			UINT uKey = itemKey->GetInt("key");
			AddHotKey(szName, uKey, uModifier1, uModifier2,true,true);
		}
	}
}
