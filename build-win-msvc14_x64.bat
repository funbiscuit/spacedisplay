@echo off
if not exist "cmake_win_path.txt" (
echo Rename cmake_win_path.example.txt to cmake_win_path.txt and provide path to cmake.exe
exit
)

set /p cmake_path=<cmake_win_path.txt

if not exist "cmake-build-msvc14_x64-release\" mkdir cmake-build-msvc14_x64-release
if not exist "bin\" mkdir bin

cd cmake-build-msvc14_x64-release
%cmake_path% "-DQT_WIN_PATH=D:\Qt\5.14.2\msvc2015_64" -G "Visual Studio 14 2015 Win64" ..
%cmake_path% --build . --config Release --target spacedisplay_gui
copy app-gui\Release\spacedisplay_gui.exe ..\bin\spacedisplay_gui.exe
cd ..
