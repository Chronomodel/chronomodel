echo 1 - Define environnement and Qt directory
@Set "QTDIR=C:\Qt\6.1.3\mingw81_64"
@Set "PATH=C:\Qt\6.1.3\mingw81_64\bin\;C:\Qt\Tools\mingw810_64\bin;%PATH%"

set QT_QPA_PLATFORM_PLUGIN_PATH=%QTDIR%\plugins\platforms\

cd chronomodel

echo 2 - linguistic
rem lupdate Chronomodel.pro
rem lrelease Chronomodel.pro

rem qmake -project Chronomodel.pro supprime l'ancien fichier pro
rem 3 - qmake
"%QTDIR\bin\qmake.exe" C:\Users\dufresne\Chronomodel\chronomodel\Chronomodel.pro -spec win32-g++ "CONFIG+=release" "CONFIG-=qml_release" && C:/Qt/Tools/mingw810_64/bin/mingw32-make.exe qmake_all

echo 4 - Make
rem mingw32-make  -f build/Makefile qmake_all
mingw32-make  -f Makefile.Release
rem mingw32-make  -f build/Makefile release

echo 5 - Compile
mingw32-make.exe -j4

cd ..


rem Keep in memory
rem cd app_dir
rem mkdir platforms
rem xcopy qwindows.dll platforms\qwindows.dll