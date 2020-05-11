$ErrorActionPreference = 'Stop';

# comment for invalidating appveyor build cache 

$vc_arch = @(" Win64", "") #empty for Win32
$sufx = @("msvc15_x64", "msvc15_x86")

$qt_dlls = @("Qt5Core.dll","Qt5Gui.dll","Qt5Svg.dll","Qt5Widgets.dll")
$qt_plug = @("platforms\qwindows.dll")

for($i = 0; $i -lt $vc_arch.length; $i++){
    if($i -eq 0){
       $qt_path = "$env:QT_DIR_WIN_64"
    } else {
       $qt_path = "$env:QT_DIR_WIN_32"
    }
    
    $vc = $sufx[$i]
    $vc_gen_sfx = $vc_arch[$i]

    # create build dir if it was not restored from cache
    if(!(test-path "build-$vc")){ mkdir "build-$vc" }
    Set-Location "build-$vc"
    cmake -DQT_WIN_PATH="$qt_path" -DWIN32_CONSOLE="Off" -G "Visual Studio 15 2017$vc_gen_sfx" ..
    cmake --build . --config Release
    Set-Location ..

    # create folders for binaries
    mkdir "bin-$vc"
    mkdir "bin-$vc\platforms"

    # copy executable and dll's
    Move-Item "build-$vc\app-gui\Release\spacedisplay_gui.exe" "bin-$vc\spacedisplay.exe"
    for($j = 0; $j -lt $qt_dlls.length; $j++){
        $dll = $qt_dlls[$j]
        Copy-Item "$qt_path\bin\$dll" "bin-$vc\$dll"
    }
    for($j = 0; $j -lt $qt_plug.length; $j++){
        $dll = $qt_plug[$j]
        Copy-Item "$qt_path\plugins\$dll" "bin-$vc\$dll"
    }
}
