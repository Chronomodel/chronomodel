echo 1-clone project ChronoModel 
rem git clone https://github.com/Chronomodel/chronomodel.git

echo 1-2 move to dev branch v3.0
rem C:\Program Files\Git\cmd\git.exe" checkout -b v3.0 --track origin/v3.0
rem git checkhout v3.0

echo 2-configure git
rem git config --global user.name "Chrono35"
rem git config --global user.email "philippe.dufresne35+cnrs@gmail.com"

echo 3-define environnement
@Set "QTDIR=C:\Qt\6.1.3\mingw81_64"
@Set "PATH=C:%QTDIR%\bin\;C:\Qt\Tools\mingw810_64\bin;%PATH%"

set QT_QPA_PLATFORM_PLUGIN_PATH=%QTDIR%\plugins\platforms\

cd C:\Users\Lanos\Documents\GitHub\chronomodel

echo 4-linguistic
rem lupdate Chronomodel.pro
rem lrelease Chronomodel.pro

rem qmake -project Chronomodel.pro supprime l'ancien fichier pro
rem qmake
%QTDIR\bin\qmake.exe C:\Users\Lanos\Documents\GitHub\chronomodel\Chronomodel.pro -spec win32-g++ "CONFIG+=debug" "CONFIG-=qml_debug"

rem  && C:/Qt/Tools/mingw810_64/bin/mingw32-make.exe qmake_all

echo 5-make
rem mingw32-make  -f build/Makefile qmake_all
mingw32-make  -f Makefile.debug
rem mingw32-make  -f build/Makefile release

echo 6-compile
mingw32-make.exe -j4

cd ..



rem cd app_dir
rem mkdir platforms
rem xcopy qwindows.dll platforms\qwindows.dll