#include "SysTask.h"
#include <comutil.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

SysTask::SysTask()
{
	m_service.CoCreateInstance(__uuidof(TaskScheduler));
	m_service->Connect(CComVariant(),CComVariant(),CComVariant(),CComVariant());
	m_service->GetFolder(CComBSTR(_T("\\")), &m_folder);
}


SysTask::~SysTask()
{
}

CComPtr<ITaskFolder> SysTask::CreateFolder(LPCTSTR name)
{
	CComPtr<ITaskFolder> folder;
	HRESULT hr = m_service->GetFolder(CComBSTR(name), &folder);
	if (FAILED(hr))
	{
		//CComVariant(L"D:(A;;FA;;;BA)(A;;FA;;;SY)")
		m_folder->CreateFolder(CComBSTR(name), CComVariant(), &folder);
	}
	return folder;
}

HRESULT SysTask::CreateTask(CComPtr<ITaskFolder>& folder, LPCTSTR taskName, LPTSTR author, LPCTSTR path, LPCTSTR cmdline, LPTSTR time, bool bDaily)
{
	if (!folder) return S_FALSE;
	HRESULT hr;
	//setup definition
	CComPtr<ITaskDefinition> pTask;
	hr = m_service->NewTask(0, &pTask);
	//setup registration info
	CComPtr<IRegistrationInfo> reginfo;
	hr = pTask->get_RegistrationInfo(&reginfo);
	if (FAILED(hr)) return hr;
	hr = reginfo->put_Author(author);
	if (FAILED(hr)) return hr;
	//setup settings
	CComPtr<ITaskSettings> settings;
	hr = pTask->get_Settings(&settings);
	if (FAILED(hr)) return hr;
	hr = settings->put_StartWhenAvailable(VARIANT_TRUE);
	if (FAILED(hr)) return hr;

	//setup actions
	CComPtr<IActionCollection> actions;
	hr = pTask->get_Actions(&actions);
	if (FAILED(hr)) return hr;
	CComPtr<IAction> action;
	hr = actions->Create(TASK_ACTION_EXEC, &action);
	if (FAILED(hr)) return hr;
	//get execaction
	CComQIPtr<IExecAction> execAction(action);
	hr = execAction->put_Path(CComBSTR(path));
	if (FAILED(hr)) return hr;
	if (cmdline) execAction->put_Arguments(CComBSTR(cmdline));

	//set time trigger
	CComQIPtr<ITriggerCollection> pTriggerCollection;
	hr = pTask->get_Triggers(&pTriggerCollection);
	if (FAILED(hr)) return hr;

	
	CComQIPtr<ITrigger> pTrigger;
	if (bDaily)
	{
		hr = pTriggerCollection->Create(TASK_TRIGGER_DAILY, &pTrigger);
		if (FAILED(hr)) return hr;
		CComQIPtr<IDailyTrigger> pDailyTrigger;
		hr = pTrigger->QueryInterface(IID_IDailyTrigger, (void**)&pDailyTrigger);
		if (FAILED(hr)) return hr;
		hr = pDailyTrigger->put_Id(L"DailyTrigger");
		if (FAILED(hr)) return hr;
		pDailyTrigger->put_StartBoundary(time);
		pDailyTrigger->put_DaysInterval((short)1);

	}
	else
	{
		hr = pTriggerCollection->Create(TASK_TRIGGER_TIME, &pTrigger);
		if (FAILED(hr)) return hr;
		CComQIPtr<ITimeTrigger> pTimeTrigger;
		hr = pTrigger->QueryInterface(IID_ITimeTrigger, (void**)&pTimeTrigger);
		if (FAILED(hr)) return hr;
		hr = pTimeTrigger->put_Id(L"TimeTrigger");
		if (FAILED(hr)) return hr;
		pTimeTrigger->put_StartBoundary(time);
	}
	
	//register
	CComPtr<IRegisteredTask> regTask;
	hr = folder->RegisterTaskDefinition(CComBSTR(taskName),//
		pTask,//
		TASK_CREATE_OR_UPDATE,//
		CComVariant(),//user
		CComVariant(),//pwd
		TASK_LOGON_INTERACTIVE_TOKEN,
		CComVariant(),//sddl
		&regTask
	);
	return hr;
}

HRESULT SysTask::DeleteTask(LPCTSTR tname, LPCTSTR fname)
{
	CComPtr<ITaskFolder> folder;
	HRESULT hr = m_service->GetFolder(CComBSTR(fname), &folder);
	if (FAILED(hr)) return hr;
	if (tname != NULL)
	{
		hr = folder->DeleteTask(CComBSTR(tname), 0);
		return hr;
	}
	else
	{
		CComQIPtr<IRegisteredTaskCollection> pAllTasks;
		hr = folder->GetTasks(TASK_ENUM_HIDDEN, &pAllTasks);
		if (FAILED(hr)) return hr;
		LONG lTaskCtns = 0;
		pAllTasks->get_Count(&lTaskCtns);
		
		for (LONG i = 0; i < lTaskCtns; i++)
		{
			CComQIPtr<IRegisteredTask> pTask;
			hr = pAllTasks->get_Item(_variant_t(i + 1), &pTask);
			if (FAILED(hr)) return hr;
			_bstr_t name;
			hr = pTask->get_Name(name.GetAddress());
			if (FAILED(hr)) return hr;
			hr = folder->DeleteTask(name, 0);
			if (FAILED(hr)) return hr;
		}
		hr = m_folder->DeleteFolder(CComBSTR(fname), 0);
		return hr;
	}
}

std::vector<SysTask::stSysTask> SysTask::GetTasks(LPCTSTR fname)
{
	CComPtr<ITaskFolder> folder;
	std::vector<stSysTask> vTasks;
	HRESULT hr = m_service->GetFolder(CComBSTR(fname), &folder);
	if (FAILED(hr)) return vTasks;
	

	CComQIPtr<IRegisteredTaskCollection> pAllTasks;
	hr = folder->GetTasks(TASK_ENUM_HIDDEN, &pAllTasks);
	if (FAILED(hr)) return vTasks;

	LONG lTaskCtns = 0;
	pAllTasks->get_Count(&lTaskCtns);

	for (LONG i = 0; i < lTaskCtns; i++)
	{
		bool bIsDaily = false;

		CComQIPtr<IRegisteredTask> pTask;
		hr = pAllTasks->get_Item(_variant_t(i + 1), &pTask);
		if (FAILED(hr)) continue;
		_bstr_t name;
		hr = pTask->get_Name(name.GetAddress());
		if (FAILED(hr)) continue;

		CComQIPtr<ITaskDefinition> pTaskDef;
		hr = pTask->get_Definition(&pTaskDef);
		if (FAILED(hr)) continue;

		CComQIPtr<ITriggerCollection> pTriggers;
		hr = pTaskDef->get_Triggers(&pTriggers);
		if (FAILED(hr)) continue;

		CComQIPtr<ITrigger> pTrigger;
		hr = pTriggers->get_Item(1, &pTrigger);
		TASK_TRIGGER_TYPE2 triggerType = TASK_TRIGGER_TIME;
		hr = pTrigger->get_Type(&triggerType);
		if (FAILED(hr)) continue;

		_bstr_t time;
		hr = pTrigger->get_StartBoundary(time.GetAddress());
		if (FAILED(hr)) continue;

		if (triggerType == TASK_TRIGGER_DAILY) bIsDaily = true;
		
		stSysTask task;
		task.sName = name;
		task.sTime = time;
		task.bDaily = bIsDaily;
		vTasks.push_back(task);
	}
	
	return vTasks;
	
}
