#!/bin/zsh
#
# In the unzipped folder
# Execute this script in a terminal with macOs the command:
# sh installCurves_macOS.sh
# chmod +rx should do to make sure that the file is executable by everyone
# 
clear
printf "\e[34;1m Project ChronoModel Team\n"
printf "\e[3m https://chronomodel.com/\n"
echo "author email:philippe.dufresne@univ-rennes1.fr" 
echo "date 2022-sept-12 "
printf "\e[0m-------------------------------------------------\n"
PWD=`pwd`
echo "The Present Working Directory is $PWD"

LOCALAPPDATA=~/Library/Application\ Support/CNRS/ChronoModel/Calib

echo " - Control of the possible presence of calibration curve"
if [ -d "$LOCALAPPDATA" ]   
then 
    printf "\e[34mthe dir $LOCALAPPDATA is present\e[0m\n"
    printf "Perhaps you have already installed personal curves?\n"
	printf " In this case, it is not necessary to reinstall the curves.\n"
	printf "\e[0m If you still want to delete the curves already installed."
	printf " Use the following command in a terminal.\e[0m\n"
#    printf "To remove other calibration curve do in a terminal"
	echo "rm -R ~/Library/Application\ Support/CNRS"
    
else
  #  echo "the dir $LOCALAPPDATA is not present"   
	printf " - Create the new directory for the calibration curves\n"
	echo "This bash file copies the calibration curves to the correct user folder:" 

	echo " $LOCALAPPDATA"
	echo
	
	# cd ~/Library/Application\ Support/CNRS
	

	mkdir -p "$LOCALAPPDATA"
	cp -R "$PWD/Calib/." "$LOCALAPPDATA"
    
    if [ -d "$LOCALAPPDATA" ]   
	then
    	printf "\e[32;1%smCreation success\e[0m\n"
    else
    	printf "\e[33;5%smHmmm, failed to create the calibration files directory !\e[0m\n"
    fi
    
fi



echo " - End script"