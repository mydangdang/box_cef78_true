#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "SysTask.h"

class Alarms
{
public:
	struct AlarmItem
	{
		bool bDaily;
		int nActType;
		//任务名
		std::string szName;
		//按钮名
		std::string szAct;
		//标题
		std::string szText;
		//备注
		std::string szInfo;
		//任务时间
		std::string szTime;
		//任务详情
		std::string szDetail;
		std::string szObserver;
	};
public:
	Alarms();
	~Alarms();

	bool AddAlarm(std::string szTag, std::string szName, std::string szTime, std::string szText, std::string szInfo, std::string szAct, std::string szDetail, int nActType, bool bDaily);
	void DelAlarm(std::string szName);

	std::vector<AlarmItem> GetAlarms();
	void SaveToFile();
protected:
	void InitFromIni(std::string szPath);
	bool AddSysTask(std::string szName,std::string szTime,bool bDaily);
	void DelSysTask(std::string szName);
public:
	static Alarms* Instance();
	static void Free();
protected:
	std::vector<AlarmItem> m_vAlarms;
	std::string m_szIniPath;
	SysTask* m_pTask;
	static Alarms* pThis;
};

