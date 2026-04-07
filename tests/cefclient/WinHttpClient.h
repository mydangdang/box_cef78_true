/**
 *  Copyright 2008-2010 Cheng Shi.  All rights reserved.
 *  Email: shicheng107@hotmail.com
 */

#pragma once
#pragma comment(lib, "Winhttp.lib")

#include "RegExp.h"
#include "StringProcess.h"
#include <comutil.h>
#include <windows.h>
#include <Winhttp.h>
#include <string>
using namespace std;

typedef bool (*PROGRESSPROC)(double);

static const unsigned int INT_RETRYTIMES = 1;
static wchar_t *SZ_AGENT = L"WinHttpClient";
static const int INT_BUFFERSIZE = 10240;    // Initial 10 KB temporary buffer, double if it is not enough.

class WinHttpClient
{
public:
    WinHttpClient(const wstring &url, PROGRESSPROC progressProc = NULL);
    ~WinHttpClient(void);

    // It is a synchronized method and may take a long time to finish.
    bool SendHttpRequest(const wstring &httpVerb = L"GET", bool disableAutoRedirect = false);
    wstring GetResponseHeader(void);
    wstring GetResponseContent(void);
    wstring GetResponseCharset(void);
    wstring GetResponseStatusCode(void);
    wstring GetResponseLocation(void);
    wstring GetRequestHost(void);
    const BYTE *GetRawResponseContent(void);
    unsigned int GetRawResponseContentLength(void);
    unsigned int GetRawResponseReceivedContentLength(void);
    bool SaveResponseToFile(const wstring &filePath);
    wstring GetResponseCookies(void);
    bool SetAdditionalRequestCookies(const wstring &cookies);
    bool SetAdditionalDataToSend(BYTE *data, unsigned int dataSize);
    bool UpdateUrl(const wstring &url);
    bool ResetAdditionalDataToSend(void);
    bool SetAdditionalRequestHeaders(const wstring &additionalRequestHeaders);
    bool SetRequireValidSslCertificates(bool require);
    bool SetProxy(const wstring &proxy);
    DWORD GetLastError(void);
    bool SetUserAgent(const wstring &userAgent);
    bool SetForceCharset(const wstring &charset);
    bool SetProxyUsername(const wstring &username);
    bool SetProxyPassword(const wstring &password);
    bool SetTimeouts(unsigned int resolveTimeout = 0,
                            unsigned int connectTimeout = 60000,
                            unsigned int sendTimeout = 30000,
                            unsigned int receiveTimeout = 30000);

private:
    WinHttpClient(const WinHttpClient &other);
    WinHttpClient &operator =(const WinHttpClient &other);
    bool SetProgress(unsigned int byteCountReceived);

    HINTERNET m_sessionHandle;
    bool m_requireValidSsl;
    wstring m_requestURL;
    wstring m_requestHost;
    wstring m_responseHeader;
    wstring m_responseContent;
    wstring m_responseCharset;
    BYTE *m_pResponse;
    unsigned int m_responseByteCountReceived;   // Up to 4GB.
    PROGRESSPROC m_pfProcessProc;
    unsigned int m_responseByteCount;
    wstring m_responseCookies;
    wstring m_additionalRequestCookies;
    BYTE *m_pDataToSend;
    unsigned int m_dataToSendSize;
    wstring m_additionalRequestHeaders;
    wstring m_proxy;
    DWORD m_dwLastError;
    wstring m_statusCode;
    wstring m_userAgent;
    bool m_bForceCharset;
    wstring m_proxyUsername;
    wstring m_proxyPassword;
    wstring m_location;
    unsigned int m_resolveTimeout;
    unsigned int m_connectTimeout;
    unsigned int m_sendTimeout;
    unsigned int m_receiveTimeout;
};
