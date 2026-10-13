@echo off
rem cd \Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel
rem  QtIFW_script_winOS.bat

cls
echo This batch file copy files and Build windows installer
echo Project ChronoModel Team
echo https://chronomodel.com/
echo author email:philippe.dufresne@univ-rennes.fr
echo date 2026-10-13

rem -------------------------------------------------------------------------
rem Racines de chemins : tout le reste du script se base dessus, pour éviter
rem les fautes de frappe repetees dans chaque ligne de copy.
rem -------------------------------------------------------------------------
set ROOT=C:\Users\Lanos\Documents\github\chronomodel
set PKG=%ROOT%\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant1\data
set QTVERSION=C:\Qt\6.11.2\mingw_64

if not exist "%QTVERSION%\bin\Qt6Core.dll" (
    echo ERREUR: Qt introuvable dans %QTVERSION% - verifier la variable QTVERSION
    pause
    exit /b 1
)

rem Extrait le numero de version Qt depuis QTVERSION (ex: C:\Qt\6.11.2\mingw_64 -> "6.11.2")
rem pour ne plus le retaper en dur dans le nom du fichier installeur (etape 7).
for %%A in ("%QTVERSION%\..") do set QT_NUM=%%~nxA
for /f "tokens=1-3 delims=." %%a in ("%QT_NUM%") do set QT_VER_STR=Qt%%a_%%b_%%c

echo  1 - Creation des dossiers de destination
if not exist "%PKG%" mkdir "%PKG%"
if not exist "%PKG%\platforms" mkdir "%PKG%\platforms"
if not exist "%PKG%\imageformats" mkdir "%PKG%\imageformats"

echo  2 - Copy icon
copy "%ROOT%\icon\Chronomodel.ico" "%PKG%"

echo  3 - Copy ABOUT.html files
copy "%ROOT%\deploy\ABOUT.html" "%PKG%"

rem Toolchain MinGW utilisee pour compiler (runtime GCC + OpenMP)
set MINGW=C:\Qt\Tools\mingw1310_64

echo  4 - Copy fichier lib
rem Runtime GCC : pris dans la toolchain qui a compile l'executable
copy "%MINGW%\bin\libgcc_s_seh-1.dll"   "%PKG%\."
copy "%MINGW%\bin\libstdc++-6.dll"      "%PKG%\."
copy "%MINGW%\bin\libwinpthread-1.dll"  "%PKG%\."
rem OpenMP (absent du bin du kit Qt)
copy "%MINGW%\bin\libgomp-1.dll"        "%PKG%\."
if errorlevel 1 echo ERREUR : libgomp-1.dll introuvable dans %MINGW%\bin


echo  4 - Copy fichier lib
rem Les DLL Qt sont dans "bin", pas a la racine du kit ; sans %% la variable
rem QTVERSION n'etait pas substituee et le script cherchait un dossier
rem litteralement nomme "QTVERSION" (d'ou les erreurs "Chemin d acces introuvable").

copy "%QTVERSION%\bin\Qt6Concurrent.dll" "%PKG%\."
copy "%QTVERSION%\bin\Qt6Core.dll" "%PKG%\."
copy "%QTVERSION%\bin\Qt6Gui.dll" "%PKG%\."
copy "%QTVERSION%\bin\Qt6Svg.dll" "%PKG%\."
copy "%QTVERSION%\bin\Qt6SvgWidgets.dll" "%PKG%\."
copy "%QTVERSION%\bin\Qt6Widgets.dll" "%PKG%\."

rem Les plugins sont sous "plugins\platforms" et "plugins\imageformats", pas a la racine du kit
copy "%QTVERSION%\plugins\platforms\qdirect2d.dll" "%PKG%\platforms\."
copy "%QTVERSION%\plugins\platforms\qminimal.dll" "%PKG%\platforms\."
copy "%QTVERSION%\plugins\platforms\qoffscreen.dll" "%PKG%\platforms\."
copy "%QTVERSION%\plugins\platforms\qwindows.dll" "%PKG%\platforms\."

rem "imagesformats" corrige en "imageformats" (nom reel du dossier de plugins Qt)
copy "%QTVERSION%\plugins\imageformats\*.dll" "%PKG%\imageformats\."

echo  5 - Copy file chronomodel.exe in the installer-packages-winOS\chronomodel_QtIFW.composant1
copy "%ROOT%\build\Desktop_Qt_6_11_2_MinGW_64_bit-Release\build\release\chronomodel.exe" "%PKG%"

echo  6 - Ignore - Copy file chronomodel_bash.exe in the installer-packages-winOS\chronomodel_QtIFW.composant2
rem copy C:\Users\Lanos\Documents\github\chronomodel\build\Desktop_Qt_6_9_2_MinGW_64_bit-Release\build\release\chronomodel_bash.exe C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant2\data
rem copy C:\Users\Lanos\Documents\github\chronomodel\icon\chronomodel_bash.ico C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant2\data

set VERSION=4.0.0
rem Pas d'espace autour de = : "set X = y" cree une variable "X " (avec l'espace dans le nom)
set DATE_STR=%date:~6,4%%date:~3,2%%date:~0,2%

echo  7 - Execution de binarycreator
C:\Qt\Tools\QtInstallerFramework\4.10\bin\binarycreator --offline-only -c installer-config/config.xml -p installer-packages-winOS ChronoModel_v%VERSION%_%QT_VER_STR%_win64_%DATE_STR%_Installer

echo  8 - Valide to finish
pause