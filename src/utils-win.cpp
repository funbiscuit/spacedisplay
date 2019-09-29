

#ifdef _WIN32

#include <iostream>
#include <algorithm>
#include <Windows.h>
#include "utils.h"

void open_folder_in_file_manager(const char* folder_path)
{
	std::string path = Utils::path_to_backslashes(folder_path);// replace all "bad" slashes to explorer-friendly slashes

    auto pars = string_format("/n,\"%s\"", path.c_str());
	auto parsw = str2wstr(pars);
	
    std::wcout << L"Launch explorer with args: " << parsw << '\n';
	
	ShellExecuteW(nullptr, L"open", L"explorer", parsw.c_str(), nullptr, SW_SHOWNORMAL);
}

void show_file_in_file_manager(const char* file_path)
{
    std::string path = Utils::path_to_backslashes(file_path);// replace all "bad" slashes to explorer-friendly slashes

    auto pars = string_format("/select,\"%s\"", path.c_str());
	auto parsw = str2wstr(pars);

    std::wcout << L"Launch explorer with args: " << parsw << '\n';

	ShellExecuteW(nullptr, L"open", L"explorer", parsw.c_str(), nullptr, SW_SHOWNORMAL);
}

std::wstring str2wstr(std::string const &str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring ret(len, '\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), (LPWSTR)ret.data(), (int)ret.size());
    return ret;
}

std::string wstr2str(std::wstring const &str)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0, nullptr, nullptr);
    std::string ret(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, str.c_str(), (int)str.size(), (LPSTR)ret.data(), (int)ret.size(), nullptr, nullptr);
    return ret;
}

#endif
