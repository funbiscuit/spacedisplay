@echo off
if not exist "cmake_win_path.txt" (
echo Rename cmake_win_path.example.txt to cmake_win_path.txt and provide path to cmake.exe
exit
)

set /p cmake_path=<cmake_win_path.txt

if not exist "cmake-build-msvc15_x64-release\" mkdir cmake-build-msvc15_x64-release
if not exist "bin\" mkdir bin

cd cmake-build-msvc15_x64-release
%cmake_path% -G "Visual Studio 15 2017 Win64" ..
%cmake_path% --build . --config Release --target spacedisplay
copy Release\spacedisplay.exe ..\bin\spacedisplay.exe
cd ..
