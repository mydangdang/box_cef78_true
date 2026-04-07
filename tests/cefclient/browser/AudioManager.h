#pragma once

#include <Audiopolicy.h>

class CAudioMgr
{
public:
    CAudioMgr();
    ~CAudioMgr();

public:
    HRESULT GetHResult() const { return m_hRes; }
    BOOL    SetProcessMute(DWORD Pid,BOOL bMute);

private:
    BOOL    __GetAudioSessionMgr2();

private:
    HRESULT                 m_hRes;
    IAudioSessionManager2*  m_lpAudioSessionMgr;
};