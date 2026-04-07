#include "Injecter.h"
#include "loaders.h"

BOOL Injecter::Inject(DWORD dwPID,const wchar_t* wsDLLPath)
{
	
	BYTE* image = NULL;
	DWORD imageSize = 0;
	if (!ReadFileData(wsDLLPath, &image, &imageSize)) return FALSE;

	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD, FALSE, dwPID);
	if (hProc == NULL) return FALSE;

	const int bits = GetProcessBits(hProc);
	if (bits == 0) return FALSE;

	const IMAGE_NT_HEADERS* ntHeader = GetNtHeader(image, imageSize);
	if (ntHeader == NULL) return FALSE;
	
	if ((ntHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 && bits != 32) || (ntHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 && bits != 64)) return FALSE;
	

	BYTE* loader = NULL;
	size_t loaderSize = 0;
	if (bits == 32) {
		loaderSize = sizeof(loader_x86);
		loader = (BYTE*)loader_x86;
	}
	else {
		loaderSize = sizeof(loader_x64);
		loader = (BYTE*)loader_x64;
	}

	// allocate remote memory for loader shellcode + dll
	const size_t remoteShellcodeSize = loaderSize + imageSize;
	void* remoteShellcode = VirtualAllocEx(hProc, 0, remoteShellcodeSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (remoteShellcode == NULL) return FALSE;
	
	SIZE_T numWritten = 0;
	if (!WriteProcessMemory(hProc, remoteShellcode, loader, loaderSize, &numWritten) ||
		!WriteProcessMemory(hProc, (BYTE*)remoteShellcode + loaderSize, image, imageSize, &numWritten)) return FALSE;

	HANDLE hThread = NULL;
	if ((GetCurrentProcessBits() == 64) || (GetCurrentProcessBits() == bits)) {
		RTLCREATEUSERTHREAD fpRtlCreateUserThread = (RTLCREATEUSERTHREAD)GetProcAddress(GetModuleHandleA("ntdll"), "RtlCreateUserThread");
		if (fpRtlCreateUserThread == NULL) return FALSE;
		NTSTATUS sRes = fpRtlCreateUserThread(hProc, NULL, FALSE, 0, 0, 0, (LPTHREAD_START_ROUTINE)remoteShellcode, NULL, &hThread, NULL);
		if (sRes != 0)
			return FALSE;
	}
	if (hThread) return TRUE;

	return FALSE;
}

const IMAGE_NT_HEADERS* Injecter::GetNtHeader(const BYTE* image, const DWORD imageSize)
{
	const IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)image;
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		return NULL;
	}
	const IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)((ULONG_PTR)image + dosHeader->e_lfanew);
	if ((BYTE*)ntHeader < image) {
		return NULL;
	}
	if ((BYTE*)ntHeader > (image + imageSize - sizeof(IMAGE_NT_HEADERS))) {
		return NULL;
	}
	if (ntHeader->Signature != IMAGE_NT_SIGNATURE) {
		return NULL;
	}
	return ntHeader;
}

BOOL Injecter::ReadFileData(const WCHAR* filename, BYTE** buff, DWORD* size)
{
	if (filename == NULL || buff == NULL || size == NULL)
		return FALSE;

	HANDLE hFile = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	// get size of file
	LARGE_INTEGER liSize;
	if (!GetFileSizeEx(hFile, &liSize)) {
		CloseHandle(hFile);
		return FALSE;
	}
	if (liSize.HighPart > 0) {
		CloseHandle(hFile);
		return FALSE;
	}

	// read entire file into memory
	*buff = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, liSize.LowPart);
	if (*buff == NULL) {
		CloseHandle(hFile);
		return FALSE;
	}
	BYTE* buffPtr = *buff;
	DWORD numLeft = liSize.LowPart;
	DWORD numRead = 0;
	while (numLeft > 0) {
		if (!ReadFile(hFile, buffPtr, numLeft, &numRead, NULL)) {
			CloseHandle(hFile);
			HeapFree(GetProcessHeap(), 0, *buff);
			return FALSE;
		}
		numLeft -= numRead;
		buffPtr += numRead;
	}
	*size = liSize.LowPart;
	CloseHandle(hFile);
	return TRUE;
}

int Injecter::GetProcessBits(HANDLE hProc)
{
	BOOL iswow64 = FALSE;
	FP_IsWow64Process fpIsWow64Process = (FP_IsWow64Process)(GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process"));
	if (fpIsWow64Process == NULL)
		return 32;
	if (fpIsWow64Process(hProc, &iswow64)) {
		return iswow64 ? 32 : 64;
	}
	return 32;
}

int Injecter::GetCurrentProcessBits()
{
	SYSTEM_INFO si = { 0 };
	GetSystemInfo(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		return 64;
	else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		return 32;
	return 32;
}

DWORD Injecter::GetThreadIdFromHandle(HANDLE hThread)
{
	// vista+ only
	GETTHREADID fpGetThreadId = (GETTHREADID)GetProcAddress(GetModuleHandleA("kernel32"), "GetThreadId");
	if (fpGetThreadId)
		return fpGetThreadId(hThread);
	return 0;
}
