

#ifdef _WIN32

#include <iostream>
#include <algorithm>
#include <Windows.h>
#include "utils.h"
#include "platformutils.h"

void open_folder_in_file_manager(const char* folder_path)
{
	std::string path = Utils::path_to_backslashes(folder_path);// replace all "bad" slashes to explorer-friendly slashes

    auto pars = string_format("/n,\"%s\"", path.c_str());
	auto parsw = PlatformUtils::str2wstr(pars);
	
    std::wcout << L"Launch explorer with args: " << parsw << '\n';
	
	ShellExecuteW(nullptr, L"open", L"explorer", parsw.c_str(), nullptr, SW_SHOWNORMAL);
}

void show_file_in_file_manager(const char* file_path)
{
    std::string path = Utils::path_to_backslashes(file_path);// replace all "bad" slashes to explorer-friendly slashes

    auto pars = string_format("/select,\"%s\"", path.c_str());
	auto parsw = PlatformUtils::str2wstr(pars);

    std::wcout << L"Launch explorer with args: " << parsw << '\n';

	ShellExecuteW(nullptr, L"open", L"explorer", parsw.c_str(), nullptr, SW_SHOWNORMAL);
}

#endif
