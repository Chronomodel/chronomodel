#!/bin/sh

# -------------------------------------------------------
#	Usage : 
#	1) cd /path/to/chronomodel
#	2) sh build_all_mac.sh 0.1 /path/to/qt/bin/
# -------------------------------------------------------

# -------------------------------------------------------
#	Arguments checks
# -------------------------------------------------------
if [ $# -lt 2 ]; then
  echo "$1: please give the version number as argument!"
  echo "$2: please give your Qt bin path (absolute)! For example : /Users/your_name/Qt5.2.1/5.2.1/clang_64/bin"
  exit 2
fi

# -------------------------------------------------------
#	Create useful variables for the paths
# -------------------------------------------------------
ROOT_PATH=$(dirname $0)
BUNDLE_PATH=Release/Chronomodel.app
QT_BIN_PATH=$2
VERSION=$1

# -------------------------------------------------------
#	Clean last compilation files
# -------------------------------------------------------
rm -rf ${BUNDLE_PATH}

cd $ROOT_PATH


# -------------------------------------------------------
#	Create XCode project in Release mode (.pro file is in debug mode by default)
# -------------------------------------------------------
#${QT_BIN_PATH}/qmake -spec macx-xcode "CONFIG+=release" $ROOT_PATH/Chronomodel.pro

# -------------------------------------------------------
#	Compile XCode project
#	Important note : important post-compilation steps are defined in the .pro file:
#	copying resource files as license, icons...
# ------------------------------------------------------
#xcodebuild -configuration Release ONLY_ACTIVE_ARCH=NO | grep -A 5 error

# -------------------------------------------------------
#	Qt utility "macdeployqt" : deploy Chronomodel's Qt dependencies inside the bundle
# ------------------------------------------------------
#${QT_BIN_PATH}/macdeployqt $BUNDLE_PATH

# -------------------------------------------------------
#	Create DMG
# ------------------------------------------------------
#ln -s /Applications ./Release/Applications
#mkdir ./Release/.background
#cp deploy/mac/dmg_back.png ./Release/.background/background.png
#hdiutil create -volname Chronomodel -srcfolder ./Release/ -ov -format UDZO Chronomodel.dmg
hdiutil attach Chronomodel.dmg
echo '
   tell application "Finder"
     tell disk "Chronomodel"
           open
           
           	set theXOrigin to 100
			set theYOrigin to 100
			set theWidth to 512
			set theHeight to 512
			
			set theBottomRightX to (theXOrigin + theWidth)
			set theBottomRightY to (theYOrigin + theHeight)
			
			tell container window
				set current view to icon view
				set toolbar visible to false
				set statusbar visible to false
				set the bounds to {theXOrigin, theYOrigin, theBottomRightX, theBottomRightY}
				set statusbar visible to false
			end tell
           
			set opts to the icon view options of container window
			tell opts
				set icon size to 72
				set arrangement to not arranged

			end tell

			set background picture of opts to file ".background:background.png"

           set file_list to every file
           set position of item "Chronomodel.app" of container window to {160, 100}
           set position of item "Applications" of container window to {360, 100}
           
           update without registering applications
           delay 2
     end tell
   end tell
' | osascript



exit 0
