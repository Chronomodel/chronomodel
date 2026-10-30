@echo off
rem version du 2026-10-30
rem ne pas mettre de blanc autour de = ("set X = y" cree une variable "X " avec l'espace dans le nom)
rem penser a compiler chronomodel.exe en Release avant
rem   cd \Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel
rem   QtIFW_script_winOs.bat
rem _________________________

rem Console en UTF-8 pour afficher les memes pictogrammes que le script macOS
chcp 65001 >nul
setlocal EnableExtensions
cls

rem ---------------------------- A VERIFIER ---------------------------------
set VERSION=4.0.0
set QT_BIN_PATH=C:\Qt\Tools\QtInstallerFramework\4.10\bin
set QTVERSION=C:\Qt\6.11.2\mingw_64
rem Toolchain MinGW utilisee pour compiler (runtime GCC + OpenMP)
set MINGW=C:\Qt\Tools\mingw1310_64

set ROOT=C:\Users\Lanos\Documents\github\chronomodel
set RELEASE_PATH=%ROOT%\build\Desktop_Qt_6_11_2_MinGW_64_bit-Release\build\release

set ROOT_PATH=%~dp0
if "%ROOT_PATH:~-1%"=="\" set ROOT_PATH=%ROOT_PATH:~0,-1%
set EXE=%RELEASE_PATH%\chronomodel.exe

set DEST=%ROOT_PATH%\installer-packages-winOS\chronomodel_QtIFW.composant1\data
rem -------------------------------------------------------------------------

echo ➡️  1 - Verification des chemins
if not exist "%QT_BIN_PATH%\binarycreator.exe" ( echo ❌ QtIFW introuvable : %QT_BIN_PATH% & goto :echec )
if not exist "%QTVERSION%\bin\Qt6Core.dll"     ( echo ❌ Qt introuvable dans %QTVERSION% - verifier la variable QTVERSION & goto :echec )
if not exist "%MINGW%\bin\libgomp-1.dll"       ( echo ❌ Toolchain MinGW introuvable ou sans OpenMP : %MINGW%\bin & goto :echec )
if not exist "%EXE%"                           ( echo ❌ Executable introuvable : %EXE% ^(compiler d'abord en Release^) & goto :echec )

echo ➡️  2 - Copy the ChronoModel files
if not exist "%DEST%\platforms"    mkdir "%DEST%\platforms"
if not exist "%DEST%\imageformats" mkdir "%DEST%\imageformats"
rem Suppression de l'ancien executable : une copie ratee ne laisse pas un .exe perime dans data
if exist "%DEST%\chronomodel.exe" del /q "%DEST%\chronomodel.exe"

call :copie "%EXE%"                                  "%DEST%"   || goto :echec
call :copie "%ROOT%\icon\Chronomodel.ico"            "%DEST%"   || goto :echec
call :copie "%ROOT%\deploy\ABOUT.html"               "%DEST%"   || goto :echec

rem Runtime GCC : pris dans la toolchain qui a compile l'executable
call :copie "%MINGW%\bin\libgcc_s_seh-1.dll"         "%DEST%"   || goto :echec
call :copie "%MINGW%\bin\libstdc++-6.dll"            "%DEST%"   || goto :echec
call :copie "%MINGW%\bin\libwinpthread-1.dll"        "%DEST%"   || goto :echec
rem OpenMP (absent du bin du kit Qt)
call :copie "%MINGW%\bin\libgomp-1.dll"              "%DEST%"   || goto :echec

rem DLL Qt (dans "bin" du kit)
for %%L in (Concurrent Core Gui Svg SvgWidgets Widgets) do (
    call :copie "%QTVERSION%\bin\Qt6%%L.dll" "%DEST%" || goto :echec
)

rem Plugins Qt
for %%P in (qdirect2d qminimal qoffscreen qwindows) do (
    call :copie "%QTVERSION%\plugins\platforms\%%P.dll" "%DEST%\platforms" || goto :echec
)
call :copie "%QTVERSION%\plugins\imageformats\*.dll" "%DEST%\imageformats" || goto :echec

rem _________________________
rem echo ➡️  - Copy the ChronoModel_bash files
rem copy "%ROOT%\build\Desktop_Qt_6_9_2_MinGW_64_bit-Release\build\release\chronomodel_bash.exe" "%ROOT_PATH%\installer-packages-winOS\chronomodel_QtIFW.composant2\data"
rem copy "%ROOT%\icon\chronomodel_bash.ico" "%ROOT_PATH%\installer-packages-winOS\chronomodel_QtIFW.composant2\data"

echo ➡️  3 - Detection automatique de la version de Qt et de Windows
rem Version Qt : extraite de QTVERSION (ex: C:\Qt\6.11.2\mingw_64 -> 6.11.2)
for %%A in ("%QTVERSION%\..") do set QT_NUM=%%~nxA
for /f "tokens=1-3 delims=." %%a in ("%QT_NUM%") do (
    set QT_VER_STR=Qt%%a_%%b_%%c
    set QT_KIT=Qt_%%a_%%b_%%c
)
echo    Qt detecte : %QT_VER_STR%
rem Le dossier de build doit correspondre au kit dont on copie les DLL
echo %RELEASE_PATH% | findstr /c:"%QT_KIT%" >nul || echo ⚠️  RELEASE_PATH ne contient pas %QT_KIT% : l'executable n'a peut-etre pas ete compile avec ce Qt

set WIN_TAG=win64
echo    Cible Windows : %WIN_TAG%

rem Date AAAAMMJJ independante des parametres regionaux (%%date%% en depend)
for /f %%D in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd"') do set DATE_FILE=%%D
if not defined DATE_FILE set DATE_FILE=%date:~6,4%%date:~3,2%%date:~0,2%

set INSTALLER=ChronoModel_v%VERSION%_%QT_VER_STR%_%WIN_TAG%_%DATE_FILE%_Installer

echo ➡️  4 - Executing binarycreator : %INSTALLER%
cd /d "%ROOT_PATH%"
"%QT_BIN_PATH%\binarycreator.exe" --offline-only -c installer-config/config.xml -p installer-packages-winOS "%INSTALLER%"
if errorlevel 1 ( echo ❌ binarycreator a echoue & goto :echec )

rem _________________________
echo ✅ 5 - Terminé : %ROOT_PATH%\%INSTALLER%.exe
pause
exit /b 0

rem -------------------------------------------------------------------------
:copie
rem Copie silencieuse ; arret du script si la source manque ou si la copie echoue
if not exist "%~1" ( echo ❌ Fichier introuvable : %~1 & exit /b 1 )
copy /y "%~1" "%~2\" >nul || ( echo ❌ Echec de la copie : %~1 & exit /b 1 )
exit /b 0

:echec
pause
exit /b 1
