$ErrorActionPreference = 'Stop';

# comment for invalidating appveyor build cache 

$vc_arch = @(" Win64", "") #empty for Win32
$sufx = @("msvc15_x64", "msvc15_x86")

$qt_dlls = @("Qt5Core.dll","Qt5Gui.dll","Qt5Svg.dll","Qt5Widgets.dll")
$qt_plug = @("platforms\qwindows.dll","styles\qwindowsvistastyle.dll")

$cmd_sfx = @("", "-cmd") #empty for normal version
$cmd_var = @("Off", "On")

for($i = 0; $i -lt $vc_arch.length; $i++){
    if($i -eq 0){
       $qt_path = "$env:QT_DIR_WIN_64"
    } else {
       $qt_path = "$env:QT_DIR_WIN_32"
    }
    
    $vc = $sufx[$i]
    $vc_gen_sfx = $vc_arch[$i]

    for($k = 0; $k -lt $cmd_var.length; $k++) {
        $cs = $cmd_sfx[$k]
        $cv = $cmd_var[$k]
        # create build dir if it was not restored from cache
        if(!(test-path "build$cs-$vc")){ mkdir "build$cs-$vc" }
        Set-Location "build$cs-$vc"
        cmake -DQT_WIN_PATH="$qt_path" -DWIN32_CONSOLE="$cv" -G "Visual Studio 15 2017$vc_gen_sfx" ..
        cmake --build . --config Release
        Set-Location ..

        # create folders for binaries
        mkdir "bin$cs-$vc"
        mkdir "bin$cs-$vc\platforms"
        mkdir "bin$cs-$vc\styles"

        # copy executable and dll's
        Move-Item "build$cs-$vc\app-gui\Release\spacedisplay_gui.exe" "bin$cs-$vc\spacedisplay.exe"
        for($j = 0; $j -lt $qt_dlls.length; $j++){
            $dll = $qt_dlls[$j]
            Copy-Item "$qt_path\bin\$dll" "bin$cs-$vc\$dll"
        }
        for($j = 0; $j -lt $qt_plug.length; $j++){
            $dll = $qt_plug[$j]
            Copy-Item "$qt_path\plugins\$dll" "bin$cs-$vc\$dll"
        }
    }
}
