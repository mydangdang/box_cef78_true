#pragma once
#include <atlcomcli.h>
#include <taskschd.h>
#include <string>
#include <vector>

class SysTask
{
public:
	struct stSysTask
	{
		bool bDaily;
		std::wstring sName;
		std::wstring sTime;
	};
public:
	SysTask();
	~SysTask();

	CComPtr<ITaskFolder> CreateFolder(LPCTSTR name);
	HRESULT CreateTask(CComPtr<ITaskFolder>& folder, LPCTSTR taskName, LPTSTR author, LPCTSTR path, LPCTSTR cmdline,LPTSTR time,bool bDaily = false);
	HRESULT DeleteTask(LPCTSTR tname, LPCTSTR fname);

	std::vector<stSysTask> GetTasks(LPCTSTR fname);
protected:
	CComPtr<ITaskService> m_service;
	CComPtr<ITaskFolder> m_folder;
};

