#!/bin/bash
# ne pas mettre de blanc autour de =
# cd ~/CM/chronomodel/QtInstaller_ChronoModel/
# sh QtIFW_script_deb.sh
# _________________________

clear
echo This batch file copy files and Build windows installer
echo Project ChronoModel Team
echo https://chronomodel.com/
echo author email:philippe.dufresne@univ-rennes.fr 
echo date 2024-05-28

echo " 1 - Copy icon"
cp ~/CM/chronomodel/icon/Chronomodel.ico ~/CM/chronomodel/QtInstaller_ChronoModel/installer-packages-deb/chronomodel_QtIFW.composant1/data
#cp ~/CM/chronomodel/icon/Chronomodel_bash.ico ~/CM/chronomodel/QtInstaller_ChronoModel/installer-packages-deb/chronomodel_QtIFW.composant2/data

echo " 2 - Copy ABOUT.html files" 
cp ~/CM/chronomodel/deploy/ABOUT.html ~/CM/chronomodel/QtInstaller_ChronoModel/installer-packages-deb/chronomodel_QtIFW.composant1/data

echo " 3 - Copy file chronomodel in the installer-packages-deb/chronomodel_QtIFW.composant1"

cp ~/CM/build-Chronomodel-Desktop_Qt_6_5_3_GCC_64bit-Release/build/release/chronomodel ~/CM/chronomodel/QtInstaller_ChronoModel/installer-packages-deb/chronomodel_QtIFW.composant1/data

#echo " 5 - Copy file chronomodel_bash.exe in the installer-packages-winOS\chronomodel_QtIFW.composant2"
#copy C:\Users\Lanos\Documents\github\build-chronomodel_bash-Desktop_Qt_6_5_3_MinGW_64_bit-Release\build\release\chronomodel_bash.exe C:\Users\Lanos\Documents\github\chronomodel\QtInstaller_ChronoModel\installer-packages-winOS\chronomodel_QtIFW.composant2\data


VERSION=3.2.7
DATESTR=$(date '+%Y%m%d')
#DATE_STR = $date:~6,4%$date:~3,2%$date:~0,2%

echo " 4 - Execution de binarycreator"
~/Qt/Tools/QtInstallerFramework/4.8/bin/binarycreator --offline-only -c installer-config/config.xml -p installer-packages-deb ChronoModel_v$VERSION\_Qt6.5.3_deb_$DATESTR\_Installer

echo " 5 - Valide to finish"
#pause
