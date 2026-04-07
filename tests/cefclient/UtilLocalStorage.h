#pragma once
#include <map>
#include <string>

typedef std::map<std::wstring, std::wstring> LocalStorage;

class UtilLocalStorage
{
public:
	UtilLocalStorage();
	~UtilLocalStorage();

	bool SetLocalStorage(std::wstring szJson);
	std::wstring GetLocalStorage(std::wstring szKey);
	bool UpdateLocalStorage(std::wstring szKey, std::wstring szValue);
public:
	std::string m_szPath;
	LocalStorage m_kv;
};

extern UtilLocalStorage LSHelper;


//   LSHelper.SetLocalStorage(LR"({
// 	  "tag":"loginWin",
// 	  "url" : "html.landers.yoozhe.com/member/login",
// 	  "left" : "-1",
// 	  "top" : "-1",
// 	  "width" : "320",
// 	  "height" : "480"
// })");
//   LSHelper.UpdateLocalStorage(L"few分为", L"范德萨发");
//   std::wstring szOut = LSHelper.GetLocalStorage(L"few分为");