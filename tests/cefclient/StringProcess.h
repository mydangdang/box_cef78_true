/**
 *  Copyright 2008-2009 Cheng Shi.  All rights reserved.
 *  Email: shicheng107@hotmail.com
 */

#ifndef STRINGPROCESS_H
#define STRINGPROCESS_H

#include "RegExp.h"
#include <Windows.h>
#include <iostream>
#include <string>
#include <comutil.h>
#pragma warning(push)
#pragma warning(disable: 4127)
#include <atlcomtime.h>
#pragma warning(pop)
using namespace std;

wstring Trim(const wstring &source, const wstring &targets);

bool PrepareString(wchar_t *dest, size_t *size, const wstring &src);

wstring ReplaceString(const wstring &srcStr, const wstring &oldStr, const wstring &newStr);

int StringToInteger(const wstring &number);

wstring LowerString(const wstring &text);

wstring UpperString(const wstring &text);

wstring GetAnchorText(const wstring &anchor);

wstring GetAnchorLink(const wstring &anchor);

bool SeparateString(const wstring &content, const wstring &delimiter, vector<wstring> &result);

wstring URLEncoding(const wstring &keyword, bool convertToUTF8 = true);

unsigned int GetSeparateKeywordMatchGrade(const wstring &source, const wstring &keyword);

unsigned int GetKeywordMatchGrade(const wstring &source, const wstring & keyword);

wstring GetDateString(const COleDateTime &time, const wstring &separator = L"-", bool align = true);

wstring GetDateString(int dayOffset, const wstring &separator = L"-", bool align = true);

wstring GetTimeString(const COleDateTime &time, const wstring &separator = L":", bool align = true);

wstring MD5(const wstring &text);

wstring FilterFileName(const wstring &name);

wstring GetMagic(unsigned int length);

wstring GetHost(const wstring &url);

wstring GetValidFileName(const wstring &fileName);

#endif // STRINGPROCESS_H
