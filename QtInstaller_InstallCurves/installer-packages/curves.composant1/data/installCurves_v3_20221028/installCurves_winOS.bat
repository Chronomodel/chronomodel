@echo off

echo Project ChronoModel Team
echo https://chronomodel.com/
echo This batch file copy the calibration curves in the the good user's folder %LOCALAPPDATA%
echo author email:philippe.dufresne@univ-rennes1.fr 
echo date 2021-07-07

echo 1 - Clean the directory, if exist
rmdir /S %LOCALAPPDATA%\CNRS
echo 2 - Create a new directory for the calibration curves
mkdir  %LOCALAPPDATA%\CNRS\ChronoModel\Calib

echo 3 - Copy the Calibration curve
robocopy /s %CD%\Calib %LOCALAPPDATA%\CNRS\ChronoModel\Calib

echo 4 - Valide to finish
pause
