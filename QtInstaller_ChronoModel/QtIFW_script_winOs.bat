@echo off

rem cd \Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel
rem  QtIFW_script_winOS.bat

cls
echo This batch file copy files and Build windows installer
echo Project ChronoModel Team
echo https://chronomodel.com/
echo author email:philippe.dufresne@univ-rennes.fr 
echo date 2024-02-06

echo " 1 - Copy icon"
copy C:\Users\Lanos\Documents\github\chronomodel\icon\Chronomodel.ico C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant1\data

echo " 2 - Copy additionnal files"
robocopy /s C:\Users\Lanos\Documents\github\chronomodel\deploy\windows\additionnal_files\Qt_6_5_3 C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant1\data

echo " 3 - Copy ABOUT.html files" 
copy C:\Users\Lanos\Documents\github\chronomodel\deploy\ABOUT.html C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant1\data

echo " 4 - Copy file chronomodel.exe in the installer-packages-winOS"
copy C:\Users\Lanos\Documents\github\build-Chronomodel-Desktop_Qt_6_5_3_MinGW_64_bit-Release\build\release\chronomodel.exe C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant1\data

echo " 5 - Execution de binarycreator"
C:\Qt\Tools\QtInstallerFramework\4.6\bin\binarycreator --offline-only -c installer-config/config.xml -p installer-packages-winOS ChronoModel_Installer_WinOS

echo " 6 - Valide to finish"
pause