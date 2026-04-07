#include "string_util.h"
#include <windows.h>
#include <memory>
#include <functional>
#include <tchar.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>



namespace UtilString
{

static inline std::string &ltrim(std::string &s) { 
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(isspace)))); 
	return s; 
} 

// trim from end 
static inline std::string &rtrim(std::string &s) { 
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(isspace))).base(), s.end()); 
	return s; 
} 

std::string& Trim(std::string &s)
{
   return ltrim(rtrim(s));  
}

bool UnicodeToAnsi(const std::wstring &src, std::string & result)
{
	int ascii_size = ::WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, NULL, 0, NULL, NULL);

	if (ascii_size == 0)
	{
		return false;
	}

	std::vector<char> result_buf(ascii_size, 0);
	int result_size =::WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, &result_buf[0], ascii_size, NULL, NULL);

	if (result_size != ascii_size)
	{
		return false;
	}

	result = &result_buf[0];

	return true;
}

bool AnsiToUnicode(const std::string &src, std::wstring & result)
{
	int wide_size = ::MultiByteToWideChar (CP_ACP, 0, src.c_str(), -1, NULL, 0);

	if (wide_size == 0)
	{
		return false;
	}

	std::vector<wchar_t> result_buf(wide_size, 0);
	int result_size = MultiByteToWideChar (CP_ACP, 0, src.c_str(), -1, &result_buf[0], wide_size);

	if (result_size != wide_size)
	{
		return false;
	}

	result = &result_buf[0];

	return true;
}

bool UnicodeToUtf8(const std::wstring &src, std::string & result)
{
	int utf8_size = ::WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, NULL, 0, NULL, NULL);

	if (utf8_size == 0)
	{
		return false;
	}

	std::vector<char> result_buf(utf8_size, 0);

	int result_size = ::WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, &result_buf[0], utf8_size, NULL, NULL);

	if (result_size != utf8_size)
	{
		return false;
	}

	result = &result_buf[0];

	return true;
}

bool Utf8ToUnicode(const std::string &src, std::wstring & result)
{
	int wide_size = ::MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, NULL, 0);

	if (wide_size == 0)
	{
		return false;
	}

	std::vector<wchar_t> result_buf(wide_size, 0);

	int result_size = ::MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, &result_buf[0], wide_size);

	if (result_size != wide_size)
	{
		return false;
	}

	result = &result_buf[0];

	return true;
}

bool AnsiToUtf8(const std::string& src, std::string &result)
{
	std::wstring wstr;
	if(!AnsiToUnicode(src, wstr))
		return false;

	return UnicodeToUtf8(wstr, result);
}

bool Utf8ToAnsi(const std::string& src, std::string &result)
{
	std::wstring wstr;
	if(!Utf8ToUnicode(src, wstr))
		return false;

	return UnicodeToAnsi(wstr, result);
}

std::string SW2A(const std::wstring &src)
{
	std::string strRet;
	if(!UnicodeToAnsi(src, strRet))
		return "";

	return strRet;
}

std::wstring SA2W(const std::string &src)
{
	std::wstring strRet;
	if(!AnsiToUnicode(src, strRet))
		return L"";

	return strRet;
}

std::string SW2U(const std::wstring &src)
{
	std::string strRet;
	if(!UnicodeToUtf8(src, strRet))
		return "";

	return strRet;
}

std::wstring SU2W(const std::string &src)
{
	std::wstring strRet;
	if(!Utf8ToUnicode(src, strRet))
		return L"";

	return strRet;
}

std::string SA2U(const std::string &src)
{
	std::string strRet;
	if(!AnsiToUtf8(src, strRet))
		return "";

	return strRet;
}

std::string SU2A(const std::string &src)
{
	std::string strRet;
	if(!Utf8ToAnsi(src, strRet))
		return "";

	return strRet;
}

unsigned char ToHex( unsigned char x )
{
	return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex( unsigned char x )
{
	unsigned char y;  
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;  
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;  
	else if (x >= '0' && x <= '9') y = x - '0';  
	return y;
}

std::string UrlEncode( const std::string& str )
{
	std::string strTemp = "";  
	size_t length = str.length();  
	for (size_t i = 0; i < length; i++)  
	{  
		if (isalnum((unsigned char)str[i]) ||   
			(str[i] == '-') ||  
			(str[i] == '_') ||   
			(str[i] == '.') ||   
			(str[i] == '~'))  
			strTemp += str[i];  
		else if (str[i] == ' ')  
			strTemp += "+";  
		else  
		{  
			strTemp += '%';  
			strTemp += ToHex((unsigned char)str[i] >> 4);  
			strTemp += ToHex((unsigned char)str[i] % 16);  
		}  
	}  
	return strTemp;
}

std::string UrlDecode( const std::string& str )
{
	std::string strTemp = "";  
	size_t length = str.length();  
	for (size_t i = 0; i < length; i++)  
	{  
		if (str[i] == '+') strTemp += ' ';  
		else if (str[i] == '%')  
		{   
			unsigned char high = FromHex((unsigned char)str[++i]);  
			unsigned char low = FromHex((unsigned char)str[++i]);  
			strTemp += high*16 + low;  
		}  
		else strTemp += str[i];  
	}  
	return strTemp;
}

bool ReadFileToString(std::string file_name, std::string& fileData)
{
	std::ifstream file(file_name.c_str(),  std::ifstream::binary);
	if(file)
	{
		file.seekg(0, file.end);
		const int file_size = (int)file.tellg();
		char* file_buf = new char [file_size+1];
		memset(file_buf, 0, file_size+1);
		file.seekg(0, ios::beg);
		file.read(file_buf, file_size);
		if(file)
			fileData.append(file_buf);
		else
		{
			fileData.append(file_buf);
			return false;
		}
		file.close();
		delete []file_buf;
	}
	else
		return false;
	return true;
}

bool WriteStringToFile(string file_name,string fileData,bool bAppend)
{
	ios_base::openmode opMode = ios::binary;
	if(!bAppend)
		opMode |= std::ofstream::trunc;
	else
		opMode |= std::ofstream::app;
	ofstream file(file_name.c_str(),opMode);
	if(file)
	{
		file << fileData.c_str();
		file.close();
	}
	else
		return false;
	return true;
}

std::string FormatStdString(const std::string fmt_str ,...)
{
	int final_n, n = ((int)fmt_str.size()) * 2;
	std::string str;
	std::unique_ptr<char[]> formatted;
	va_list ap;
	while(1) {
		formatted.reset(new char[n]);
		strcpy_s(&formatted[0],n, fmt_str.c_str());
		va_start(ap, fmt_str);
		final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
		va_end(ap);
		if (final_n < 0 || final_n >= n)
			n += abs(final_n - n + 1);
		else
			break;
	}
	return std::string(formatted.get());
}

std::vector<std::string> split(std::string str,std::string pattern)
{
	std::string::size_type pos;
	std::vector<std::string> result;
	str+=pattern;
	UINT size=str.size();

	for(UINT i=0; i<size; i++)
	{
		pos=str.find(pattern,i);
		if(pos<size)
		{
			std::string s=str.substr(i,pos-i);
			if(!s.empty())
				result.push_back(s);
			i=pos+pattern.size()-1;
		}
	}
	return result;
}


static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";


static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string Base64Encdoe(std::string const& s ) {

	char const* bytes_to_encode = s.c_str();
	unsigned int in_len = s.length();

	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';

	}

	return ret;

}

std::string Base64Decode(std::string const& encoded_string) {
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

	return ret;
}

}