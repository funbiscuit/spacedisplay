#include "FileManager.h"

#include "utils.h"
#include "filepath.h"
#include "platformutils.h"

#include <Windows.h>
#include <iostream>

void FileManager::openFolder(const std::string &path) {
    std::string folder_path = path;
    FilePath::normalize(folder_path, FilePath::SlashHandling::ADD);
    auto pars = Utils::strFormat("/n,\"%s\"", folder_path.c_str());
    auto parsw = PlatformUtils::str2wstr(pars);

    std::wcout << L"Launch explorer with args: " << parsw << '\n';

    ShellExecuteW(nullptr, L"open", L"explorer", parsw.c_str(), nullptr, SW_SHOWNORMAL);
}

void FileManager::showFile(const std::string &path) {
    std::string file_path = path;
    FilePath::normalize(file_path, FilePath::SlashHandling::REMOVE);
    auto pars = Utils::strFormat("/select,\"%s\"", file_path.c_str());
    auto parsw = PlatformUtils::str2wstr(pars);

    std::wcout << L"Launch explorer with args: " << parsw << '\n';

    ShellExecuteW(nullptr, L"open", L"explorer", parsw.c_str(), nullptr, SW_SHOWNORMAL);
}

