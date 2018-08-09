version 2.0
2018-05-15

——————————————————
BUILD ON MAC
——————————————————

Pre-requisite:
- Xcode (latest version recommended)
- Qt 5 installed on your Mac (latest version recommended)
- Xcode tools installed (for pkgbuild tool, among others…)

The build_all_mac.sh contains everything to:
- create an Xcode project from the .pro file
- build the Xcode project 
- create a package ready to deploy

In your Terminal, go to the project path and then type :
sh build_all_mac.sh 2.0 /your/absolute/path/to/Qt/5.5/clang_64/bin/

Note : 1.2 is the version number. We use it when releasing official versions.
You may choose what you want instead when building by yourself.

——————————————————
ADDITIONNAL NOTES
——————————————————
Generate a MAC icon :
Use command line tool in the « icon » folder:
iconutil -c icns Chronomodel.iconset
https://developer.apple.com/library/mac/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/Optimizing/Optimizing.html#//apple_ref/doc/uid/TP40012302-CH7-SW3


Windows compilation notes:
When compiling on Windows, if you get a message like :
'<UNC path>' is an invalid current directory path. 
Then refer to this documentation to solve the problem :
https://support.microsoft.com/en-us/kb/156276
It will guide you through adding a new register key.
Without it, Qt may fail to compile the resource file Chronomodel.rc


 ——————————————————
BUILD ON UBUNTU (linux)
——————————————————
 
1 - You need to install, Qt5, QtCreator, Git and two special libraries:

 sudo apt-get update

1-1 Install Qt5
# sudo apt-get install qtdeclarative5-dev
# sudo apt-get install qtbase5-dev

1-2 Install QtCreator
#sudo apt-get install qt-sdk

sudo apt-get install build-essential

1-3 Install Git
sudo apt-get install git-gui
sudo apt-get install gitk

1-4 Install libraries for Chronomodel
sudo apt-get install fftw3*
sudo apt-get install libqt5svg5* 

reset
cd ./Chronomodel

2 - Download the code on GitHub
git clone https://github.com/Chronomodel/chronomodel.git

3 - Make the compilation
3-1 Move inside the new directory
cd chronomodel

3-2 Change the mode of all files, don't forget the last character "."
sudo chmod -R a+rwx .

3-3 - Cleaning previous release
sudo rm -r build/release

3-4 Update linguistic file
/usr/lib/x86_64-linux-gnu/qt5/bin/lupdate Chronomodel.pro
/usr/lib/x86_64-linux-gnu/qt5/bin/lrelease Chronomodel.pro

3-3 Compilation
/usr/lib/x86_64-linux-gnu/qt5/bin/qmake Chronomodel.pro
make

4 - Copy the directory Calib
sudo cp -fra deploy/Calib build/release/Calib

5 - Launch ChronoModel
./build/release/Chronomodel