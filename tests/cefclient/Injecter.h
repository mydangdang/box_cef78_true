#pragma once
#include <windows.h>

class Injecter
{
	typedef DWORD(WINAPI* GETTHREADID)(HANDLE);
	typedef BOOL(WINAPI* FP_IsWow64Process)(HANDLE, BOOL*);
	typedef LONG(NTAPI* RTLCREATEUSERTHREAD)(HANDLE, PSECURITY_DESCRIPTOR, BOOLEAN, ULONG, SIZE_T, SIZE_T, PTHREAD_START_ROUTINE, PVOID, PHANDLE, LPVOID);
public:
	static BOOL Inject(DWORD dwPID,const wchar_t* wsDLLPath);
protected:
	static const IMAGE_NT_HEADERS* GetNtHeader(const BYTE* image, const DWORD imageSize);
	static BOOL ReadFileData(const WCHAR* filename, BYTE** buff, DWORD* size);
	static int GetProcessBits(HANDLE hProc);
	static int GetCurrentProcessBits();
	static DWORD GetThreadIdFromHandle(HANDLE hThread);
};

