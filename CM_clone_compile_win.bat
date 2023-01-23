echo 1 - Clone project ChronoModel 
git clone https://github.com/Chronomodel/chronomodel.git

echo 1-2 Move to dev branch v3.0
C:\Program Files\Git\cmd\git.exe checkout -b v3.0 --track origin/v3.0

echo 2 - Configure git
git config --global user.name "Chrono35"
git config --global user.email "philippe.dufresne35+cnrs@gmail.com"

echo 3 - define environnement
@Set "QTDIR=C:\Qt\6.1.3\mingw81_64"
@Set "PATH=C:\Qt\6.1.3\mingw81_64\bin\;C:\Qt\Tools\mingw810_64\bin;%PATH%"

set QT_QPA_PLATFORM_PLUGIN_PATH=%QTDIR%\plugins\platforms\
cd chronomodel

echo 4 - linguistic
lupdate Chronomodel.pro
lrelease Chronomodel.pro


rem qmake -project Chronomodel.pro supprime l'ancien fichier pro
rem 3 - qmake
"%QTDIR\bin\qmake.exe" C:\Users\dufresne\Chronomodel\chronomodel\Chronomodel.pro -spec win32-g++ "CONFIG+=release" "CONFIG-=qml_release" && C:/Qt/Tools/mingw810_64/bin/mingw32-make.exe qmake_all

echo 4 - Make
rem mingw32-make  -f Makefile qmake_all
rem mingw32-make  -f Makefile.Debug

mingw32-make  -f Makefile.Release

echo 5 - Compile
mingw32-make.exe -j4

cd ..


rem Keep in memory
rem cd app_dir
rem mkdir platforms
rem xcopy qwindows.dll platforms\qwindows.dll