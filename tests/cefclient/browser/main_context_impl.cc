// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "tests/cefclient/browser/main_context_impl.h"

#include "include/cef_parser.h"
#include "include/cef_web_plugin.h"
#include "tests/shared/common/client_switches.h"
#include <Shlwapi.h>
#include "../RTCTX.h"
#include "../string_util.h"
#include "../IniParse.h"
#include "../MD5.h"
#include "../AES.h"
#include "tests/shared/browser/util_win.h"
#include "../Alarms.h"
#include <process.h>
#include "../FileAttribute.h"
#include "../wmic.h"
#include "../WinHttpClient.h"
#include "../ScreenRecorder.h"

namespace client {

namespace {

// The default URL to load in a browser window.
const char kDefaultUrl[] = "file://E:/Project/WebFrame/webtest/index.html";
//	const char kDefaultUrl[] = "file://E:/MyNAS/WebFrame/webtest/index.html";

// Returns the ARGB value for |color|.
cef_color_t ParseColor(const std::string& color) {
  std::string colorToLower;
  colorToLower.resize(color.size());
  std::transform(color.begin(), color.end(), colorToLower.begin(), ::tolower);

  if (colorToLower == "black")
    return CefColorSetARGB(255, 0, 0, 0);
  else if (colorToLower == "blue")
    return CefColorSetARGB(255, 0, 0, 255);
  else if (colorToLower == "green")
    return CefColorSetARGB(255, 0, 255, 0);
  else if (colorToLower == "red")
    return CefColorSetARGB(255, 255, 0, 0);
  else if (colorToLower == "white")
    return CefColorSetARGB(255, 255, 255, 255);

  // Use the default color.
  return 0;
}

}  // namespace

/*
void LoadPayProtect(MainContextImpl* pThis)
{
	if (!pThis->main_url_.empty())
	{
		std::wstring szURL = UtilString::SA2W(pThis->main_url_);

		do 
		{
			int nPos = szURL.find(L"/", 8);
			if (nPos != -1)
				szURL = szURL.substr(0, nPos);
			pThis->m_szHost = UtilString::SW2A(szURL);
			szURL += LR"(/softsubmit/checkver?ver=)";
			szURL += UtilString::SA2W(RTCTX::Version());
		
			WinHttpClient clt(szURL.c_str());
			clt.SendHttpRequest();
			wstring szContet = clt.GetResponseContent();
			if (!szContet.empty())
			{
				CefRefPtr<CefValue> value = CefParseJSON(szContet, JSON_PARSER_RFC);
				if (value.get() && value->GetType() == VTYPE_DICTIONARY)
				{
					CefRefPtr<CefDictionaryValue> infos = value->GetDictionary();
					int nCode = infos->GetInt("code");
					if (nCode) pThis->m_bEnableADS = false;
				}
			}

			if (pThis->m_bEnableADS)
			{
				std::string szPlugDLL = RTCTX::CurPath() + "\\box_blob.bin";
#ifdef _DEBUG
				std::string szReleasePath = client::DataPath() + "\\box_blobD.dat";
#else
				std::string szReleasePath = client::DataPath() + "\\box_blob.dat";
#endif
				if (!PathFileExistsA(szReleasePath.c_str()))
				{
					HANDLE hPluginDLLBin = NULL;
					HANDLE hDLLBin = CreateFileA(szPlugDLL.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if(hDLLBin == INVALID_HANDLE_VALUE) break;
					BYTE* pDatas = NULL;
					do 
					{
						BOOL bRet = false;
						DWORD dwSize = 0, dwRet = 0 , dwHigh;
						dwSize = GetFileSize(hDLLBin, &dwHigh);
						if (dwSize <= 0) break;
						pDatas = new BYTE[dwSize];
						bRet = ReadFile(hDLLBin, pDatas, dwSize, &dwRet, NULL);
						if (!bRet || dwSize != dwRet ) break;
						hPluginDLLBin = CreateFileA(szReleasePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hPluginDLLBin == INVALID_HANDLE_VALUE)
						{
							hPluginDLLBin = 0;
							break;
						}
						
#ifdef _DEBUG
						int nOffset = 0;
#else
						int nOffset = 5;
#endif
						bRet = WriteFile(hPluginDLLBin, pDatas + nOffset, dwSize - nOffset, &dwRet, NULL);
						if (!bRet || (dwSize- nOffset) != dwRet) break;
						CloseHandle(hPluginDLLBin);
						hPluginDLLBin = NULL;
						pThis->m_hPluginDLL = LoadLibraryA(szReleasePath.c_str());
					} while (0);
					if (pDatas) delete pDatas;
					if (hDLLBin) CloseHandle(hDLLBin);
					if (hPluginDLLBin) CloseHandle(hPluginDLLBin);
				}
				else
				{
					pThis->m_hPluginDLL = LoadLibraryA(szReleasePath.c_str());
				}
			}

		} while (0);

	}
	if (pThis->m_hPluginEvent) SetEvent(pThis->m_hPluginEvent);
}
*/

MainContextImpl::MainContextImpl(CefRefPtr<CefCommandLine> command_line,
	bool terminate_when_all_windows_closed, std::string szCfg)
	: command_line_(command_line),
	terminate_when_all_windows_closed_(terminate_when_all_windows_closed),
	initialized_(false),
	shutdown_(false),
	background_color_(0),
	browser_background_color_(0),
	windowless_frame_rate_(0),
	use_views_(false) {
	DCHECK(command_line_.get());

	m_InitPos.left = 200;
	m_InitPos.top = 300;
	m_InitPos.right = 1000;
	m_InitPos.bottom = 900;

	m_szInitFrameName = "1stTag";

	// Set the main URL.
	if (command_line_->HasSwitch(switches::kUrl))
		main_url_ = command_line_->GetSwitchValue(switches::kUrl);
	if (main_url_.empty())
		main_url_ = kDefaultUrl;

	int nCnts;
	char szTemp[512] = { 0 };
	nCnts = LoadStringA(GetModuleHandle(NULL), 110, szTemp, 512);
	main_url_ = std::string(szTemp, nCnts);

	memset(szTemp, 0, 512);
	nCnts = LoadStringA(GetModuleHandle(NULL), 111, szTemp, 512);
	m_szInitFrameName = std::string(szTemp, nCnts);

	memset(szTemp, 0, 512);
	nCnts = LoadStringA(GetModuleHandle(NULL), 112, szTemp, 512);
	std::string szWidth = std::string(szTemp, nCnts);

	memset(szTemp, 0, 512);
	nCnts = LoadStringA(GetModuleHandle(NULL), 113, szTemp, 512);
	std::string szHeight = std::string(szTemp, nCnts);

	int nStartWidth = atoi(szWidth.c_str()) * GetDeviceScaleFactor();
	int nStartHeight = atoi(szHeight.c_str()) * GetDeviceScaleFactor();

	m_InitPos.left = (int)((GetSystemMetrics(SM_CXSCREEN) - nStartWidth)*1.0f / 2);
	m_InitPos.top = (int)((GetSystemMetrics(SM_CYSCREEN) - nStartHeight)*1.0f / 2);
	m_InitPos.right = m_InitPos.left + nStartWidth;
	m_InitPos.bottom = m_InitPos.top + nStartHeight;



	if (command_line_->HasSwitch("clock"))
		m_szClockName = command_line_->GetSwitchValue("clock");

	std::string szAppFile = RTCTX::CurPath() + "\\app.txt";
	std::string szAppInfo;
	if (!UtilString::ReadFileToString(szAppFile, szAppInfo) || szAppInfo.empty()) szAppInfo = szCfg;

	bool bLayerd = false;
	if(!szAppInfo.empty())
	{
		CefRefPtr<CefValue> value = CefParseJSON(szAppInfo, JSON_PARSER_RFC);
		if (value.get() && value->GetType() == VTYPE_DICTIONARY)
		{
			CefRefPtr<CefDictionaryValue> infos = value->GetDictionary();
			std::string szURL = infos->GetString("url");

			if (!szURL.empty() && szURL.find("http") == -1 && szURL.find("www") == -1 && szURL.find("com") == -1 && szURL.find(":") == -1)
			{
				szURL = RTCTX::CurPath() + "/" + szURL;
			}

			main_url_ = szURL;

			m_hPluginEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			//std::thread(LoadPayProtect, this).detach();

			int nWidth = infos->GetInt("width") * GetDeviceScaleFactor();
			int nHeight = infos->GetInt("height") * GetDeviceScaleFactor();
			int nLeft = infos->GetInt("left");
			int nTop = infos->GetInt("top");
			bLayerd = infos->GetBool("windowless");
			if(infos->HasKey("tag")) m_szInitFrameName = infos->GetString("tag");

			if (nLeft <= 0)
				nLeft = (int)((GetSystemMetrics(SM_CXSCREEN) - nWidth)*1.0f / 2);
			if (nTop <= 0)
				nTop = (int)((GetSystemMetrics(SM_CYSCREEN) - nHeight)*1.0f / 2);

			m_InitPos.left = nLeft;
			m_InitPos.top = nTop;
			m_InitPos.right = nLeft + nWidth;
			m_InitPos.bottom = nTop + nHeight;
		}
	}

  // Whether windowless (off-screen) rendering will be used.
  use_windowless_rendering_ =
      command_line_->HasSwitch(switches::kOffScreenRenderingEnabled);

  if (bLayerd)
	  use_windowless_rendering_ = true;

  if (use_windowless_rendering_ &&
      command_line_->HasSwitch(switches::kOffScreenFrameRate)) {
    windowless_frame_rate_ =
        atoi(command_line_->GetSwitchValue(switches::kOffScreenFrameRate)
                 .ToString()
                 .c_str());
  }

  // Whether transparent painting is used with windowless rendering.
  bool use_transparent_painting =
      use_windowless_rendering_ &&
      command_line_->HasSwitch(switches::kTransparentPaintingEnabled);

  if (bLayerd) use_transparent_painting = true;

#if defined(OS_WIN)
  // Shared texture is only supported on Windows.
  shared_texture_enabled_ =
      use_windowless_rendering_ &&
      command_line_->HasSwitch(switches::kSharedTextureEnabled);
#endif

  external_begin_frame_enabled_ =
      use_windowless_rendering_ &&
      command_line_->HasSwitch(switches::kExternalBeginFrameEnabled);

  if (windowless_frame_rate_ <= 0) {
// Choose a reasonable default rate based on the OSR mode.
#if defined(OS_WIN)
    windowless_frame_rate_ = shared_texture_enabled_ ? 60 : 30;
#else
    windowless_frame_rate_ = 30;
#endif
  }

#if defined(OS_WIN) || defined(OS_LINUX)
  // Whether the Views framework will be used.
  use_views_ = command_line_->HasSwitch(switches::kUseViews);

  if (use_windowless_rendering_ && use_views_) {
    LOG(ERROR)
        << "Windowless rendering is not supported by the Views framework.";
    use_views_ = false;
  }

  if (use_views_ && command_line->HasSwitch(switches::kHideFrame) &&
      !command_line_->HasSwitch(switches::kUrl)) {
    // Use the draggable regions test as the default URL for frameless windows.
    main_url_ = "http://tests/draggable";
  }
#endif  // defined(OS_WIN) || defined(OS_LINUX)

  if (command_line_->HasSwitch(switches::kBackgroundColor)) {
    // Parse the background color value.
    background_color_ =
        ParseColor(command_line_->GetSwitchValue(switches::kBackgroundColor));
  }

  

  if (background_color_ == 0 && !use_views_) {
    // Set an explicit background color.
    background_color_ = CefColorSetARGB(25, 255, 0, 0);
  }

  // |browser_background_color_| should remain 0 to enable transparent painting.
  if (!use_transparent_painting) {
    browser_background_color_ = background_color_;
  }

 
  const std::string& cdm_path =
      command_line_->GetSwitchValue(switches::kWidevineCdmPath);
  if (!cdm_path.empty()) {
    // Register the Widevine CDM at the specified path. See comments in
    // cef_web_plugin.h for details. It's safe to call this method before
    // CefInitialize(), and calling it before CefInitialize() is required on
    // Linux.
    CefRegisterWidevineCdm(cdm_path, NULL);
  }
}

MainContextImpl::~MainContextImpl() {
  // The context must either not have been initialized, or it must have also
  // been shut down.
  DCHECK(!initialized_ || shutdown_);

//   if (m_hPluginDLL)
//   {
// 	  FreeLibrary(m_hPluginDLL);
// 	  m_hPluginDLL = NULL;
//   }
//   if (m_hPluginEvent)
//   {
// 	  CloseHandle(m_hPluginEvent);
// 	  m_hPluginEvent = NULL;
//   }
}

std::string MainContextImpl::GetConsoleLogPath() {
  return GetAppWorkingDirectory() + "console.log";
}

std::string MainContextImpl::GetMainURL() {
  return main_url_;
}

void MainContextImpl::UpdateMemStorage(const std::string& sTag, const std::string& sValue)
{
	m_JsMemStorage[sTag] = sValue;
}

std::string MainContextImpl::GetMemStorage(const std::string& sTag)
{
	return m_JsMemStorage[sTag];
}

std::string MainContextImpl::GetVersion()
{
	return RTCTX::Version();
}

void MainContextImpl::SetUID(std::string szUID)
{
	std::string szReportCfg = client::DataPath() + "\\reportcfg";
	CIniParse reportInfo((char*)szReportCfg.c_str(), true);
	reportInfo.SetString("report","uid", szUID.c_str());
}

std::string MainContextImpl::GetBaseInfo()
{
    if (m_szBaseInfo.empty())
    {
        std::string szSid, szCid, szGid,szCpuid,szMacid,szVer;

		std::string szCfg = client::DataPath() + "\\cfg";
		CIniParse baseInfo((char*)szCfg.c_str(),true);
        char szTmp[256] = { 0 };
        int nSize = 256;
        baseInfo.GetString("channel", "s",szTmp, nSize);
        szSid = std::string(szTmp, nSize);
		if (!szSid.empty()) szSid = AES::DecryptionAES(szSid);
		else szSid = "35";

        nSize = 256;
		baseInfo.GetString("channel", "c", szTmp, nSize);
        szCid = std::string(szTmp, nSize);
		if (!szCid.empty()) szCid = AES::DecryptionAES(szCid);
		else szCid = "0";

		nSize = 256;
		baseInfo.GetString("channel", "g", szTmp, nSize);
		szGid = std::string(szTmp, nSize);
		if (!szGid.empty()) szGid = AES::DecryptionAES(szGid);
		else szCid = "0";

        szMacid = RTCTX::MAC();
        szCpuid = RTCTX::SysDiskID();

        CHAR szMD5[64] = { 0 };
        if(!szMacid.empty())
        {
            __MD5_LEGACY_API((LPVOID)szMacid.c_str() , szMacid.size() , (PUCHAR)szMD5);
			string	szOutput;
			for (int i = 0; i < 16; i++)
			{
				CHAR		Temp[4];
				sprintf_s(Temp, 4, "%02X", (UCHAR)szMD5[i]);
				szOutput += Temp;
			}
            szMacid = szOutput;
        }

		if (!szCpuid.empty())
		{
			__MD5_LEGACY_API((LPVOID)szCpuid.c_str(), szCpuid.size(), (PUCHAR)szMD5);
			string	szOutput;
			for (int i = 0; i < 16; i++)
			{
				CHAR		Temp[4];
				sprintf_s(Temp, 4, "%02X", (UCHAR)szMD5[i]);
				szOutput += Temp;
			}
			szCpuid = szOutput;
		}
        CefRefPtr<CefDictionaryValue> resp = CefDictionaryValue::Create();
        resp->SetString("sid", szSid);
        resp->SetString("cid", szCid);
        resp->SetString("gid", szGid);
		resp->SetString("mac", szMacid);
        resp->SetString("cpuid", szCpuid);
		resp->SetString("ver", GetVersion());


		m_szSID = szSid;
		m_szMAC = szMacid;
		m_szCPUID = szCpuid;

		CefRefPtr<CefValue> value = CefValue::Create();
		value->SetDictionary(resp);
        m_szBaseInfo = CefWriteJSON(value, JSON_WRITER_DEFAULT);
    }
	return m_szBaseInfo;
}

std::wstring MainContextImpl::GetHardwarInfo()
{
	if (!m_szHardwarInfo.empty()) return m_szHardwarInfo;

	WMIC wmi;
	CefRefPtr<CefDictionaryValue> hardware = CefDictionaryValue::Create();
	hardware->SetString("mac", RTCTX::MAC());
	hardware->SetString("cpuid", RTCTX::SysDiskID());
	hardware->SetString("os", RTCTX::OSVer());
	hardware->SetBool("x64", RTCTX::IsX64());

	CefRefPtr<CefDictionaryValue> board = CefDictionaryValue::Create();
	WMIC_BaseBoard boardInfo = wmi.BaseBoard();
	board->SetString("name", boardInfo.name);
	board->SetString("manufacturer", boardInfo.manufacturer);
	board->SetString("serialNumber", boardInfo.serialNumber);
	hardware->SetDictionary("motherborad", board);

	CefRefPtr<CefDictionaryValue> bios = CefDictionaryValue::Create();
	WMIC_BIOS biosInfo = wmi.BIOS();
	bios->SetString("manufacturer", biosInfo.manufacturer);
	bios->SetString("serialNumber", biosInfo.serialNumber);
	hardware->SetDictionary("bios", bios);

	std::vector<WMIC_DiskDrive> disks = wmi.DiskDrive();
	if (disks.size())
	{
		CefRefPtr<CefDictionaryValue> disk = CefDictionaryValue::Create();
		disk->SetString("name", disks[0].name);
		disk->SetString("deviceID", disks[0].deviceID);
		disk->SetString("serialNumber", disks[0].serialNumber);
		hardware->SetDictionary("disk", disk);
	}

	std::vector<WMIC_VideoController> vedios = wmi.VideoController();
	if (vedios.size())
	{
		CefRefPtr<CefDictionaryValue> vedio = CefDictionaryValue::Create();
		vedio->SetString("name", vedios[0].name);
		vedio->SetString("deviceID", vedios[0].deviceID);
		hardware->SetDictionary("vediocard", vedio);
	}

	CefRefPtr<CefValue> value = CefValue::Create();
	value->SetDictionary(hardware);
	m_szHardwarInfo = CefWriteJSON(value, JSON_WRITER_DEFAULT);
	return m_szHardwarInfo;
}

void MainContextImpl::SetInfo(const std::string& szSID, const std::string& szCID, const std::string& szGID)
{
	std::string szSid, szCid, szGid;
	if (!szSID.empty()) szSid = AES::EncryptionAES(szSID);
	if (!szCID.empty()) szCid = AES::EncryptionAES(szCID);
	if (!szGID.empty()) szGid = AES::EncryptionAES(szGID);

	std::string szCfg = client::DataPath() + "\\cfg";

	CIniParse baseInfo((char*)szCfg.c_str(), true);
	if (!szGid.empty()) baseInfo.SetString("channel", "s", szSid.c_str());
	if (!szCid.empty()) baseInfo.SetString("channel", "c", szCid.c_str());
	if (!szGid.empty()) baseInfo.SetString("channel", "g", szGid.c_str());

	m_szBaseInfo.clear();

	char szTmp[256] = { 0 };
	int nSize = 256;
	baseInfo.GetString("channel", "s", szTmp, nSize);
	szSid = std::string(szTmp, nSize);
	if (!szSid.empty()) szSid = AES::DecryptionAES(szSid);

	nSize = 256;
	baseInfo.GetString("channel", "c", szTmp, nSize);
	szCid = std::string(szTmp, nSize);
	if (!szCid.empty()) szCid = AES::DecryptionAES(szCid);

	nSize = 256;
	baseInfo.GetString("channel", "g", szTmp, nSize);
	szGid = std::string(szTmp, nSize);
	if (!szGid.empty()) szGid = AES::DecryptionAES(szGid);

	std::string szMacid = RTCTX::MAC();
	std::string szCpuid = RTCTX::SysDiskID();

	CHAR szMD5[64] = { 0 };
	if (!szMacid.empty())
	{
		__MD5_LEGACY_API((LPVOID)szMacid.c_str(), szMacid.size(), (PUCHAR)szMD5);
		string	szOutput;
		for (int i = 0; i < 16; i++)
		{
			CHAR		Temp[4];
			sprintf_s(Temp, 4, "%02X", (UCHAR)szMD5[i]);
			szOutput += Temp;
		}
		szMacid = szOutput;
	}

	if (!szCpuid.empty())
	{
		__MD5_LEGACY_API((LPVOID)szCpuid.c_str(), szCpuid.size(), (PUCHAR)szMD5);
		string	szOutput;
		for (int i = 0; i < 16; i++)
		{
			CHAR		Temp[4];
			sprintf_s(Temp, 4, "%02X", (UCHAR)szMD5[i]);
			szOutput += Temp;
		}
		szCpuid = szOutput;
	}
	CefRefPtr<CefDictionaryValue> resp = CefDictionaryValue::Create();
	resp->SetString("sid", szSid);
	resp->SetString("cid", szCid);
	resp->SetString("gid", szGid);
	resp->SetString("mac", szMacid);
	resp->SetString("cpuid", szCpuid);
	resp->SetString("ver", GetVersion());

	CefRefPtr<CefValue> value = CefValue::Create();
	value->SetDictionary(resp);
	m_szBaseInfo = CefWriteJSON(value, JSON_WRITER_DEFAULT);
}

std::string MainContextImpl::GetInitFrameName()
{
	return m_szInitFrameName;
}

RECT MainContextImpl::GetInitPos()
{
	return m_InitPos;
}

cef_color_t MainContextImpl::GetBackgroundColor() {
  return background_color_;
}

bool MainContextImpl::UseViews() {
  return use_views_;
}

bool MainContextImpl::UseWindowlessRendering() {
  return use_windowless_rendering_;
}

bool MainContextImpl::TouchEventsEnabled() {
  return command_line_->GetSwitchValue("touch-events") == "enabled";
}

void MainContextImpl::PopulateSettings(CefSettings* settings) {
#if defined(OS_WIN) || defined(OS_LINUX)
  settings->multi_threaded_message_loop =
      command_line_->HasSwitch(switches::kMultiThreadedMessageLoop);
#endif

  if (!settings->multi_threaded_message_loop) {
    settings->external_message_pump =
        command_line_->HasSwitch(switches::kExternalMessagePump);
  }

  CefString(&settings->cache_path) =
      command_line_->GetSwitchValue(switches::kCachePath);

  if (use_windowless_rendering_)
    settings->windowless_rendering_enabled = true;

  if (browser_background_color_ != 0)
    settings->background_color = browser_background_color_;
}

void MainContextImpl::PopulateBrowserSettings(CefBrowserSettings* settings) {
  settings->windowless_frame_rate = windowless_frame_rate_;
  settings->web_security = STATE_DISABLED;
  settings->file_access_from_file_urls = STATE_ENABLED;
  settings->universal_access_from_file_urls = STATE_ENABLED;
  settings->local_storage = STATE_ENABLED;

  if (browser_background_color_ != 0)
    settings->background_color = browser_background_color_;
}

void MainContextImpl::PopulateOsrSettings(OsrRendererSettings* settings) {
  settings->show_update_rect =
      command_line_->HasSwitch(switches::kShowUpdateRect);

#if defined(OS_WIN)
  settings->shared_texture_enabled = shared_texture_enabled_;
#endif
  settings->external_begin_frame_enabled = external_begin_frame_enabled_;
  settings->begin_frame_rate = windowless_frame_rate_;

  if (browser_background_color_ != 0)
    settings->background_color = browser_background_color_;
}

RootWindowManager* MainContextImpl::GetRootWindowManager() {
  DCHECK(InValidState());
  return root_window_manager_.get();
}

bool MainContextImpl::SysShutDownPlan(int nBrowserID,bool bAdd, int nLeftMinites, int nWarninigMinites)
{
	bool bRes = true;
	if (bAdd) bRes = ShutDownPlan::Instance()->StartPlan(this, nBrowserID,nLeftMinites, nWarninigMinites);
	else ShutDownPlan::Instance()->CancelPlan();
	return bRes;
}

void MainContextImpl::OnPlanWarning(int nBrowserID,int nLeftMinites)
{
	if (!CURRENTLY_ON_MAIN_THREAD()) {
		return MAIN_POST_CLOSURE(base::Bind(&MainContextImpl::OnPlanWarning,base::Unretained(this), nBrowserID,nLeftMinites));
	}

	if(root_window_manager_.get())
	{
		std::string szTag = root_window_manager_->GetWindowForBrowser(nBrowserID)->GetBrowserTagFromID(nBrowserID);

		CefRefPtr<CefDictionaryValue> msg = CefDictionaryValue::Create();
		msg->SetInt("leftminites", nBrowserID);
		root_window_manager_->PostCustomMsg(nBrowserID,szTag,"onSysShutingdown", msg);
	}
}

std::string MainContextImpl::GetClockName()
{
	return m_szClockName;
}

void MainContextImpl::OnClock(std::string szName)
{
	if (!CURRENTLY_ON_MAIN_THREAD()) {
		return MAIN_POST_CLOSURE(base::Bind(&MainContextImpl::OnClock, base::Unretained(this), szName));
	}

	if (root_window_manager_.get())
	{
		std::string szCfg = client::DataPath() + "\\cfg";
		CIniParse alarmInfos((char*)szCfg.c_str(), true);
	
		int nAlarmCnt = 0;
		std::string szKey;
		std::string szSectionName = "alarm";
		nAlarmCnt = alarmInfos.GetInt((char*)szSectionName.c_str(), "counts", 0);

		for (int i = 0; i < nAlarmCnt; i++)
		{
			char szIndex[20] = { 0 };
			sprintf_s(szIndex, 20, "%d", i);
			szSectionName.append(szIndex);

			char szTmp[512] = { 0 };
			int nSize = 512;
			alarmInfos.GetString((char*)szSectionName.c_str(), "name", szTmp, nSize);
			szKey = std::string(szTmp, nSize);
			if(szKey != szName) continue;
			else
			{
				nSize = 512;
				alarmInfos.GetString((char*)szSectionName.c_str(), "observer", szTmp, nSize);
				std::string szTag = std::string(szTmp, nSize);

				nSize = 512;
				alarmInfos.GetString((char*)szSectionName.c_str(), "text", szTmp, nSize);
				std::string szTitle = std::string(szTmp, nSize);

				nSize = 512;
				alarmInfos.GetString((char*)szSectionName.c_str(), "info", szTmp, nSize);
				std::string szInfo = std::string(szTmp, nSize);

				nSize = 512;
				alarmInfos.GetString((char*)szSectionName.c_str(), "detail", szTmp, nSize);
				std::string szDetail = std::string(szTmp, nSize);

				nSize = 512;
				alarmInfos.GetString((char*)szSectionName.c_str(), "observer", szTmp, nSize);
				std::string szObserver = std::string(szTmp, nSize);

				int nDaily = alarmInfos.GetInt((char*)szSectionName.c_str(), "daily", 1);

				CefRefPtr<RootWindow> rootWindow = root_window_manager_->GetWindowForBrowser(szTag);
				if (rootWindow)
				{
					CefRefPtr<CefDictionaryValue> msg = CefDictionaryValue::Create();
					msg->SetString("title", UtilString::SA2W(szTitle).c_str());
					msg->SetString("info", UtilString::SA2W(szInfo).c_str());
					msg->SetString("detail", UtilString::SA2W(szDetail).c_str());
					msg->SetString("key", UtilString::SA2W(szName).c_str());
					msg->SetString("observer", UtilString::SA2W(szObserver).c_str());
					rootWindow->PostCustomMsg(rootWindow->GetBrowser()->GetIdentifier(), szTag, "onClock", msg);

					if (!nDaily) Alarms::Instance()->DelAlarm(szName);
				}
				break;
			}
		}
	}
}

void MainContextImpl::OnRecordScreenStop(std::string szTagName)
{
	if (!CURRENTLY_ON_MAIN_THREAD())
	{
		return MAIN_POST_CLOSURE(base::Bind(&MainContextImpl::OnRecordScreenStop, base::Unretained(this), szTagName));
	}

	ScreenRecorder::Instance()->OnEndRecord();
	CefRefPtr<RootWindow> rootWindow;
	if (!szTagName.empty()) 
		rootWindow = root_window_manager_->GetWindowForBrowser(szTagName);
	else 
		rootWindow = root_window_manager_->GetActiveRootWindow();
	if (rootWindow)
	{
		CefRefPtr<CefDictionaryValue> msg = CefDictionaryValue::Create();
		rootWindow->PostCustomMsg(rootWindow->GetBrowser()->GetIdentifier(), szTagName, "onRecordingStop", msg);
	}
}

bool MainContextImpl::Initialize(const CefMainArgs& args,
                                 const CefSettings& settings,
                                 CefRefPtr<CefApp> application,
                                 void* windows_sandbox_info) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!initialized_);
  DCHECK(!shutdown_);

 
  if (!CefInitialize(args, settings, application, windows_sandbox_info))
    return false;

  // Need to create the RootWindowManager after calling CefInitialize because
  // TempWindowX11 uses cef_get_xdisplay().
  root_window_manager_.reset(
      new RootWindowManager(terminate_when_all_windows_closed_));

  DWORD dwModifier = 0;
#ifdef _DEBUG
  dwModifier = VK_MENU;
#endif
 
  hotKeyManager.m_pObserver = this;
  hotKeyManager.ReadFromLocalStorage();
  stHotKey* pHotKey = hotKeyManager.GetHotKey(HK_BOSS);
  if (!pHotKey) hotKeyManager.AddHotKey(HK_BOSS, VK_F8, dwModifier);
  pHotKey = hotKeyManager.GetHotKey(HK_TOPFULL);
  if (!pHotKey) hotKeyManager.AddHotKey(HK_TOPFULL, VK_F11, dwModifier);
  pHotKey = hotKeyManager.GetHotKey(HK_SNAPSHOT);
  if (!pHotKey) hotKeyManager.AddHotKey(HK_SNAPSHOT, VK_F7, dwModifier);

  initialized_ = true;
  return true;
}

void MainContextImpl::Shutdown() {
	m_oBaseTrayIcon.Delete();

  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(initialized_);
  DCHECK(!shutdown_);

  root_window_manager_.reset();

  CefShutdown();

  shutdown_ = true;
}

void MainContextImpl::OnHotKey(stHotKey* pHotKey)
{
	if (!CURRENTLY_ON_MAIN_THREAD()) {
		return MAIN_POST_CLOSURE(base::Bind(&MainContextImpl::OnHotKey, base::Unretained(this), pHotKey));
	}
	
	if (pHotKey->szName == HK_TOPFULL)
	{
		if (root_window_manager_.get())  root_window_manager_->OnTopFullHotKey();
	}
	else if (pHotKey->szName == HK_BOSS)
	{
		if (root_window_manager_.get())  root_window_manager_->OnBossHotKey();
	}
	else if (pHotKey->szName == HK_SNAPSHOT)
	{
		std::string szExePath = RTCTX::CurPath() + "\\snapshot.exe";
		if (PathFileExistsA(szExePath.c_str()))
		{
			SHELLEXECUTEINFOA sei = {sizeof(SHELLEXECUTEINFO)};
			sei.fMask = SEE_MASK_NOCLOSEPROCESS;
			sei.lpVerb = "open";
			sei.lpFile = szExePath.c_str();
			sei.nShow = SW_SHOWNORMAL;
			ShellExecuteExA(&sei);
		}
	}
	else if (pHotKey->szName == HK_STOPRECORDSCREEN)
	{
		ScreenRecorder::Instance()->Stop();
	}
}

void MainContextImpl::CreateTrayICON(HINSTANCE hInst, HWND hWnd)
{
	m_oBaseTrayIcon.SetIcon(hInst, 131);
	m_oBaseTrayIcon.SetNotifyWnd(hWnd, WM_USER+1001);
	m_oBaseTrayIcon.Show();
	std::string szProductName;
	char szModulePath[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, szModulePath, MAX_PATH);
	std::string szExePath = szModulePath;
	UtilFileAttribute::GetProductName(szExePath, szProductName);
	m_oBaseTrayIcon.SetToolTip(UtilString::SA2W(szProductName).c_str());
}

void MainContextImpl::ShowBalloonTip(std::wstring szTip)
{
	m_oBaseTrayIcon.SetBallonToolTip(0, szTip.c_str());
}

HMODULE MainContextImpl::GetPlugInDLL()
{
	return NULL;
// 	if (m_hPluginEvent)
// 	{
// 		DWORD dwRet = WaitForSingleObject(m_hPluginEvent, 1000);
// 		if (dwRet == WAIT_OBJECT_0)
// 		{
// 			CloseHandle(m_hPluginEvent);
// 			m_hPluginEvent = NULL;
// 		}
// 	}
// 	return m_hPluginDLL;
}


}  // namespace client
