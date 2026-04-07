#include "Alarms.h"
#include "IniParse.h"
#include "string_util.h"
#include "RTCTX.h"
#include <time.h>
#include "../shared/browser/util_win.h"

#define AUTHOR L"GameBox"
#define TASKFOLD L"GameBox"

Alarms* Alarms::pThis = NULL;

Alarms::Alarms()
{
	m_pTask = new SysTask;

	std::string szCfg = client::DataPath() + "\\cfg";
	InitFromIni(szCfg.c_str());
}

Alarms::~Alarms()
{
	if (m_pTask)
	{
		delete m_pTask;
		m_pTask = NULL;
	}
}

bool Alarms::AddAlarm(std::string szTag,std::string szName, std::string szTime, std::string szText, std::string szInfo, std::string szAct, std::string szDetail, int nActType, bool bDaily)
{
	int nOldIdx = -1;
	AlarmItem tmpItem;
	bool bFound = false;
	for (UINT i = 0; i < m_vAlarms.size(); i++)
	{
		if (m_vAlarms[i].szName == szName)
		{
			nOldIdx = i;
			tmpItem = m_vAlarms[i];
			m_vAlarms[i].szObserver = szTag;
			m_vAlarms[i].nActType = nActType;
			m_vAlarms[i].szAct = szAct;
			m_vAlarms[i].szDetail = szDetail;
			m_vAlarms[i].bDaily = bDaily;
			m_vAlarms[i].szTime = szTime;
			m_vAlarms[i].szText = szText;
			m_vAlarms[i].szInfo = szInfo;
			DelSysTask(szName);
			bFound = true;
			break;
		}
	}
	if (AddSysTask(szName, szTime, bDaily))
	{
		if (!bFound)
		{
			tmpItem.szObserver = szTag;
			tmpItem.szName = szName;
			tmpItem.szTime = szTime;
			tmpItem.szText = szText;
			tmpItem.szInfo = szInfo;
			tmpItem.szDetail = szDetail;
			tmpItem.szAct = szAct;
			tmpItem.nActType = nActType;
			tmpItem.bDaily = bDaily;
			m_vAlarms.push_back(tmpItem);
		}
		SaveToFile();
		return true;
	}
	else
	{
		if (bFound) m_vAlarms[nOldIdx] = tmpItem;
		return false;
	}
}

void Alarms::DelAlarm(std::string szName)
{
	std::vector<AlarmItem>::iterator it = m_vAlarms.begin();
	for (; it != m_vAlarms.end(); it++)
	{
		if (it->szName == szName)
		{
			m_vAlarms.erase(it);
			break;
		}
	}
	SaveToFile();
	DelSysTask(szName);
}

std::vector<Alarms::AlarmItem> Alarms::GetAlarms()
{
	return m_vAlarms;
}

void Alarms::SaveToFile()
{
	if (m_szIniPath.empty()) return;
	CIniParse alarmInfos((char*)m_szIniPath.c_str(),true);
	std::string szSectionName = "alarm";

	alarmInfos.SetInt((char*)szSectionName.c_str(), "counts", m_vAlarms.size());

	for (UINT i = 0; i < m_vAlarms.size(); i++)
	{
		szSectionName = "alarm";
		char szIndex[20] = { 0 };
		sprintf_s(szIndex, 20, "%d", i);
		szSectionName.append(szIndex);

		alarmInfos.SetString((char*)szSectionName.c_str(), "observer", m_vAlarms[i].szObserver.c_str());
		alarmInfos.SetString((char*)szSectionName.c_str(), "name", m_vAlarms[i].szName.c_str());
		alarmInfos.SetString((char*)szSectionName.c_str(), "text", m_vAlarms[i].szText.c_str());
		alarmInfos.SetString((char*)szSectionName.c_str(), "time", m_vAlarms[i].szTime.c_str());
		alarmInfos.SetString((char*)szSectionName.c_str(), "info", m_vAlarms[i].szInfo.c_str());
		alarmInfos.SetString((char*)szSectionName.c_str(), "detail", m_vAlarms[i].szDetail.c_str());
		alarmInfos.SetString((char*)szSectionName.c_str(), "actname", m_vAlarms[i].szAct.c_str());
		alarmInfos.SetInt((char*)szSectionName.c_str(), "acttype", m_vAlarms[i].nActType);
		if (m_vAlarms[i].bDaily) alarmInfos.SetInt((char*)szSectionName.c_str(), "daily", 1);
		else alarmInfos.SetInt((char*)szSectionName.c_str(), "daily", 0);
	}
}

time_t String2Time(const std::string & time_string)
{
	struct tm tm_res = { 0 };
	time_t time_res;

	int i = sscanf(time_string.c_str(), "%d-%d-%dT%d:%d:%d",
		&(tm_res.tm_year),
		&(tm_res.tm_mon),
		&(tm_res.tm_mday),
		&(tm_res.tm_hour),
		&(tm_res.tm_min),
		&(tm_res.tm_sec));

	tm_res.tm_year -= 1900;
	tm_res.tm_mon--;
	tm_res.tm_isdst = -1;

	time_res = mktime(&tm_res);

	return time_res;
}

void Alarms::InitFromIni(std::string szPath)
{
	m_szIniPath = szPath;
	CIniParse alarmInfos((char*)szPath.c_str(),true);

	int nAlarmCnt = 0;
	std::string szSectionName = "alarm";
	nAlarmCnt = alarmInfos.GetInt((char*)szSectionName.c_str(),"counts", 0);
	
	for (UINT i = 0; i < nAlarmCnt; i++)
	{
		AlarmItem item;
		szSectionName = "alarm";
		char szIndex[20] = { 0 };
		sprintf_s(szIndex, 20, "%d", i);
		szSectionName.append(szIndex);
	
		char szTmp[512] = { 0 };

		int nSize = 512;
		alarmInfos.GetString((char*)szSectionName.c_str(), "observer", szTmp, nSize);
		item.szObserver = std::string(szTmp, nSize);

		nSize = 512;
		alarmInfos.GetString((char*)szSectionName.c_str(), "name", szTmp, nSize);
		item.szName = std::string(szTmp, nSize);

		nSize = 512;
		alarmInfos.GetString((char*)szSectionName.c_str(), "time", szTmp, nSize);
		item.szTime = std::string(szTmp, nSize);

		nSize = 512;
		alarmInfos.GetString((char*)szSectionName.c_str(), "text", szTmp, nSize);
		item.szText = std::string(szTmp, nSize);

		nSize = 512;
		alarmInfos.GetString((char*)szSectionName.c_str(),"info", szTmp, nSize);
		item.szInfo = std::string(szTmp, nSize);

		nSize = 512;
		alarmInfos.GetString((char*)szSectionName.c_str(), "detail", szTmp, nSize);
		item.szDetail = std::string(szTmp, nSize);

		nSize = 512;
		alarmInfos.GetString((char*)szSectionName.c_str(), "actname", szTmp, nSize);
		item.szAct = std::string(szTmp, nSize);

		item.nActType = alarmInfos.GetInt((char*)szSectionName.c_str(), "acttype", 0);

		int nDaily = 0;
		nDaily = alarmInfos.GetInt((char*)szSectionName.c_str(), "daily", 0);
		if (nDaily) item.bDaily = true;
		else
		{
			item.bDaily = false;
			time_t tClock = String2Time(item.szTime);
			time_t tNow = time(NULL);
			if (tClock <= tNow)
			{
				DelSysTask(item.szName);
				continue;
			}
		}
		m_vAlarms.push_back(item);
	}
	SaveToFile();
}

bool Alarms::AddSysTask(std::string szName, std::string szTime, bool bDaily)
{
	CComPtr<ITaskFolder> folder = m_pTask->CreateFolder(TASKFOLD);
	if (!folder) return false;
	std::wstring tName = UtilString::SA2W(szName);
	//L"2020-06-11T16:40:00"
	std::wstring tTime = UtilString::SA2W(szTime);
	
	std::string szPath = RTCTX::CurPath();
	szPath = szPath + "\\Clock.exe";

	std::wstring tCmdLine = L" " + tName;
	HRESULT hRes = m_pTask->CreateTask(folder, tName.c_str() ,AUTHOR, UtilString::SA2W(szPath).c_str(), tCmdLine.c_str(),(LPTSTR)tTime.c_str(), bDaily);
	if (hRes == S_OK) return true;
	return false;
}

void Alarms::DelSysTask(std::string szName)
{
	std::wstring tName = UtilString::SA2W(szName);
	m_pTask->DeleteTask(tName.c_str(), TASKFOLD);
}

Alarms* Alarms::Instance()
{
	if (!pThis) pThis = new Alarms;
	return pThis;
}

void Alarms::Free()
{
	if (pThis)
	{
		delete pThis;
		pThis = NULL;
	}
}


