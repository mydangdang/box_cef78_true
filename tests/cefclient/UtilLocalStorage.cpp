#include "UtilLocalStorage.h"
#include "..\shared\browser\util_win.h"
#include <fstream>
#include "include\cef_parser.h"
#include <iostream>
#include <xlocale>


UtilLocalStorage::UtilLocalStorage()
{
	m_szPath = client::DataPath() + "\\local.dat";
	std::wstring szJson;

	HANDLE hFile = CreateFileA(m_szPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwSize = GetFileSize(hFile,NULL);
		if(dwSize)
		{
			LPBYTE pBytes = new BYTE[dwSize];
			memset(pBytes, 0, dwSize);
			DWORD dwReaded = 0;
			DWORD dwTotalRead = 0;
			do
			{
				dwReaded = 0;
				if (!ReadFile(hFile, pBytes, dwSize - dwTotalRead, &dwReaded, 0))
					break;
				dwTotalRead += dwReaded;
			} while (dwTotalRead < dwSize);

			szJson = std::wstring((wchar_t*)pBytes, dwTotalRead / sizeof(wchar_t));
			delete pBytes;
			if (!szJson.empty())
			{
				CefRefPtr<CefValue> value = CefParseJSON(szJson, JSON_PARSER_RFC);
				if (value)
				{
					CefRefPtr<CefDictionaryValue> dic = value->GetDictionary();
					std::vector<CefString> keys;
					dic->GetKeys(keys);
					for each (auto key in keys)
						m_kv[key.ToWString()] = dic->GetString(key);
				}
			}
		}
	}
	CloseHandle(hFile);
	
}


UtilLocalStorage::~UtilLocalStorage()
{
}

bool UtilLocalStorage::SetLocalStorage(std::wstring szJson)
{
	CefRefPtr<CefValue> value = CefParseJSON(szJson, JSON_PARSER_RFC);
	if (!value) return false;

	//±£¡Ùhotkey;
	std::wstring szHotKeys = m_kv[L"hotkeys"];
	m_kv.clear();
	m_kv[L"hotkeys"] = szHotKeys;

	CefRefPtr<CefDictionaryValue> dic = value->GetDictionary();
	std::vector<CefString> keys;
	dic->GetKeys(keys);
	for each(auto key in keys) 
		m_kv[key.ToWString()] = dic->GetString(key);

	HANDLE hFile = CreateFileA(m_szPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	DWORD dwWritted = 0;
	BOOL bWrite = WriteFile(hFile, szJson.c_str(), szJson.size() * sizeof(wchar_t), &dwWritted, 0);
	CloseHandle(hFile);
	return bWrite;
}

std::wstring UtilLocalStorage::GetLocalStorage(std::wstring szKey)
{
	if (!szKey.empty())
	{
		if (m_kv.find(szKey) != m_kv.end())
			return m_kv[szKey];
		else return L"";
	}
	else {
		CefRefPtr<CefDictionaryValue> dic = CefDictionaryValue::Create();
		for each (auto lsInfo in m_kv)
		{
			if(lsInfo.first != L"hotkeys")
				dic->SetString(lsInfo.first, lsInfo.second);
		}
		CefRefPtr<CefValue> value = CefValue::Create();
		value->SetDictionary(dic);
		CefString szJson = CefWriteJSON(value, JSON_WRITER_DEFAULT);
		return szJson.ToWString();
	}
}

bool UtilLocalStorage::UpdateLocalStorage(std::wstring szKey, std::wstring szValue)
{
	m_kv[szKey] = szValue;

	CefRefPtr<CefDictionaryValue> dic = CefDictionaryValue::Create();
	for each (auto lsInfo in m_kv) 
		dic->SetString(lsInfo.first, lsInfo.second);
	CefRefPtr<CefValue> value = CefValue::Create();
	value->SetDictionary(dic);
	CefString szJson = CefWriteJSON(value, JSON_WRITER_PRETTY_PRINT);

	HANDLE hFile = CreateFileA(m_szPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	DWORD dwWritted = 0;
	BOOL bWrite = WriteFile(hFile, szJson.ToWString().c_str(), szJson.size() * sizeof(wchar_t), &dwWritted, 0);
	CloseHandle(hFile);
	return bWrite;
}

UtilLocalStorage LSHelper;
