#pragma once

// Add GDI+ support to MFC or WTL application.
//
// Include this file in StdAfx.h
//
// MFC: Add a GDIPlusEnv variable to your application class to start and stop GDI+.
// ATL: Create a GDIPlusEnv local variable in _tWinMain.
// Constructor starts, destructor stops.



// Ensure that GdiPlus header files work properly with MFC DEBUG_NEW and STL header files.
// Q317799: PRB: Microsoft Foundation Classes DEBUG_NEW Does Not Work with GDI+

#define iterator _iterator

#ifdef _DEBUG

static int nGdiPlusObjects = 0;

namespace Gdiplus
{
	namespace DllExports
	{
		#include <GdiplusMem.h>
	};

#ifndef _GDIPLUSBASE_H
#define _GDIPLUSBASE_H
	class GdiplusBase
	{
	public:
		void (operator delete)(void* in_pVoid)
		{
			nGdiPlusObjects--;
			DllExports::GdipFree(in_pVoid);
		}

		void* (operator new)(size_t in_size)
		{
			nGdiPlusObjects++;
			return DllExports::GdipAlloc(in_size);
		}

		void (operator delete[])(void* in_pVoid)
		{
			nGdiPlusObjects--;
			DllExports::GdipFree(in_pVoid);
		}

		void* (operator new[])(size_t in_size)
		{
			nGdiPlusObjects++;
			return DllExports::GdipAlloc(in_size);
		}

		void * (operator new)(size_t nSize, LPCSTR /*lpszFileName*/, int /*nLine*/)
		{
			nGdiPlusObjects++;
			return DllExports::GdipAlloc(nSize);
		}

		void operator delete(void* p, LPCSTR /*lpszFileName*/, int /*nLine*/)
		{
			nGdiPlusObjects--;
			DllExports::GdipFree(p);
		}

	};
#endif // #ifndef _GDIPLUSBASE_H
}
#endif // #ifdef _DEBUG

#include <gdiplus.h>
#ifdef _MFC_VER
#include <afx.h>
#endif
#undef iterator

#ifndef _MT_GDI_PLUS_ENV_H_
#define _MT_GDI_PLUS_ENV_H_

using namespace Gdiplus;
#pragma comment (lib, "gdiplus.lib")

// GDI+环境设置
class GdiPlusEnv
{
public:
	GdiPlusEnv(): m_Token(0)	{ Gdiplus::GdiplusStartupInput input; Gdiplus::GdiplusStartup(& m_Token, & input, NULL); }
	~GdiPlusEnv()
	{
		Gdiplus::GdiplusShutdown(m_Token);
#ifdef _DEBUG
#ifdef _MFC_VER
		if (nGdiPlusObjects > 0)
			afxDump << _T(">>> GdiPlus Memory Leaks: ") << nGdiPlusObjects << _T(" objects! <<<\n");
		else if (nGdiPlusObjects < 0)
			afxDump << _T(">>> GdiPlus Multiple Deletes: ") << -nGdiPlusObjects << _T(" objects! <<<\n");
#endif
#ifdef _ATL_VER
		if (nGdiPlusObjects > 0)
			ATLTRACE2(_T(">>> GdiPlus Memory Leaks: %d objects! <<<\n"), nGdiPlusObjects);
		else if (nGdiPlusObjects < 0)
			ATLTRACE2(_T(">>> GdiPlus Multiple Deletes: %d objects! <<<\n"), -nGdiPlusObjects);
#endif
#endif
	}
private:
	// The token we get from GDI+
	ULONG_PTR m_Token;
};

static GdiPlusEnv gdiPlusEnv; // GDI+环境配置，  自动执行GDI+环境设置


static int GdiPGetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;
	UINT  size = 0;
	ImageCodecInfo* pImageCodecInfo = NULL;
	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;
	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;
	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}
	free(pImageCodecInfo);
	return -1;
}

static void OnQuickSave(Bitmap *p, LPTSTR szFilePath)
{
	CLSID Clsid;
	GdiPGetEncoderClsid(L"image/png", &Clsid);
	if (p)
		p->Save(szFilePath, &Clsid);
}

#endif // _MT_GDI_PLUS_ENV_H_
