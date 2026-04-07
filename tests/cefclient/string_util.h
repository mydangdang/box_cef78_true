#pragma once
#include <vector>
#include <string>

using namespace std;

namespace UtilString 
{
    std::string& Trim(std::string &s);

	std::vector<std::string> split(std::string str,std::string pattern);
	std::string FormatStdString( const std::string fmt_str ,... );

	std::string UrlEncode(const std::string& str);  
	std::string UrlDecode(const std::string& str);

	std::string SW2A(const std::wstring &src);
	std::wstring SA2W(const std::string &src);
	std::string SW2U(const std::wstring &src);
	std::wstring SU2W(const std::string &src);
	std::string SA2U(const std::string &src);
	std::string SU2A(const std::string &src);

	std::string Base64Encdoe(std::string const& s);
	std::string Base64Decode(std::string const& s);

	bool ReadFileToString(std::string file_name, std::string& fileData);
	bool WriteStringToFile(std::string file_name,std::string fileData,bool bAppend);
};