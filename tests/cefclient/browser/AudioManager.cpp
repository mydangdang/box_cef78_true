#include <windows.h>
#include <mmdeviceapi.h>
#include <Psapi.h>
#include "AudioManager.h"
#include <audiopolicy.h>
#include <atlcomcli.h>

#pragma comment(lib, "Psapi.lib")

CAudioMgr::CAudioMgr()
    : m_hRes( ERROR_SUCCESS )
    , m_lpAudioSessionMgr( NULL )
{
}

CAudioMgr::~CAudioMgr()
{
}

BOOL CAudioMgr::SetProcessMute(DWORD Pid, BOOL bMute)
{
    if ( !this->__GetAudioSessionMgr2() || m_lpAudioSessionMgr == NULL )
    {
        return FALSE;
    }

    CComPtr<IAudioSessionEnumerator> pAudioSessionEnumerator;
    m_hRes = m_lpAudioSessionMgr->GetSessionEnumerator(&pAudioSessionEnumerator);
    if ( FAILED(m_hRes) || pAudioSessionEnumerator == NULL )
    {
        return FALSE;
    }

    int nCount = 0;
    m_hRes = pAudioSessionEnumerator->GetCount(&nCount);

    for ( int i = 0; i < nCount; ++i )
    {
        CComPtr<IAudioSessionControl> pAudioSessionControl;
        m_hRes = pAudioSessionEnumerator->GetSession(i, &pAudioSessionControl);
        if ( FAILED(m_hRes) || pAudioSessionControl == NULL )
        {
            continue;
        }

        CComQIPtr<IAudioSessionControl2> pAudioSessionControl2(pAudioSessionControl);
        if ( pAudioSessionControl2 == NULL )
        {
            continue;
        }

        DWORD dwPid = 0;
        m_hRes = pAudioSessionControl2->GetProcessId(&dwPid);
        if( FAILED(m_hRes) )
        {
            continue;
        }

        if ( dwPid == Pid )
        {
            CComQIPtr<ISimpleAudioVolume> pSimpleAudioVolume(pAudioSessionControl2);
            if ( pSimpleAudioVolume == NULL )
            {
                continue;
            }

            m_hRes = pSimpleAudioVolume->SetMute(bMute, NULL);
            break;
        }
    }

    return SUCCEEDED(m_hRes);
}

BOOL CAudioMgr::__GetAudioSessionMgr2()
{
    if ( m_lpAudioSessionMgr == NULL )
    {
        CComPtr<IMMDeviceEnumerator> pMMDeviceEnumerator;

        m_hRes = pMMDeviceEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL);
        if (  FAILED(m_hRes) || (pMMDeviceEnumerator == NULL) )
        {
            return FALSE;
        }

        CComPtr<IMMDevice> pDefaultDevice;
        m_hRes = pMMDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultDevice);
        if( FAILED(m_hRes) || pDefaultDevice == NULL )
        {
            return FALSE;
        }

        m_hRes = pDefaultDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&m_lpAudioSessionMgr);
        if ( FAILED(m_hRes) || (m_lpAudioSessionMgr == NULL) )
        {
            return FALSE;
        }
    }

    return TRUE;
}