@echo off

rem cd \Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel
rem  QtIFW_script_winOS.bat

cls
echo This batch file copy files and Build windows installer
echo Project ChronoModel Team
echo https://chronomodel.com/
echo author email:philippe.dufresne@univ-rennes.fr 
echo date 2025-08-29

echo "➡️ 1 - Copy icon"
copy C:\Users\Lanos\Documents\github\chronomodel\icon\Chronomodel.ico C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant1\data
copy C:\Users\Lanos\Documents\github\chronomodel\icon\chronomodel_bash.ico C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant2\data

echo "➡️ 2 - Copy additionnal files"
robocopy /s C:\Users\Lanos\Documents\github\chronomodel\deploy\windows\additionnal_files\Qt_6_8_2 C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant1\data

echo "➡️ 3 - Copy ABOUT.html files" 
copy C:\Users\Lanos\Documents\github\chronomodel\deploy\ABOUT.html C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant1\data

echo "➡️ 4 - Copy file chronomodel.exe in the installer-packages-winOS\chronomodel_QtIFW.composant1"
copy C:\Users\Lanos\Documents\github\chronomodel\build\Desktop_Qt_6_8_2_MinGW_64_bit-Release\build\release\chronomodel.exe C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant1\data

echo "➡️ 5 - Copy file chronomodel_bash.exe in the installer-packages-winOS\chronomodel_QtIFW.composant2"
copy C:\Users\Lanos\Documents\github\chronomodel\build\Desktop_Qt_6_8_2_MinGW_64_bit-Release\build\release\chronomodel_bash.exe C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant2\data

set VERSION=3.3.0
set DATE_STR = %date:~6,4%%date:~3,2%%date:~0,2%

echo "➡️ 6 - Execution de binarycreator"
C:\Qt\Tools\QtInstallerFramework\4.6\bin\binarycreator --offline-only -c installer-config/config.xml -p installer-packages-winOS ChronoModel_v%VERSION%_Qt6.8.2_win64_%date:~6,4%%date:~3,2%%date:~0,2%_Installer

echo "✅ 7 - Valide to finish"
pause