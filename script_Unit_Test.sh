#
# Execute this script in a terminal with MacOs the command:
# sh script_test.sh
#
clear

BUILD_PATH=/Users/dufresne/ChronoModel-SoftWare/chronomodel/Unit_Test/Unit_Test_Chronomodel/Buid_Test
QT_BIN_PATH=/Users/dufresne/Qt/5.7/clang_64/bin

echo "Cleaning the last compilation"

rm -rf ${BUILD_PATH}
echo "Creation directory"
mkdir -p ${BUILD_PATH}

echo "Doing a XCode project -- do it the first time"
${QT_BIN_PATH}/qmake  -spec macx-xcode "CONFIG+=debug" Unit_Test_Chronomodel/Unit_Test_Chronomodel.pro

echo "Doing the Makefile"
${QT_BIN_PATH}/qmake Unit_Test_Chronomodel/Unit_Test_Chronomodel.pro -o ${BUILD_PATH}
echo "Doing a 2 qmake"
cd ${BUILD_PATH}
${QT_BIN_PATH}/qmake 
echo "Execute make"
#make ${BUILD_PATH}/Makefile -o ${BUILD_PATH}/Unit_Test_Chronomodel.app
make 

echo "Execute inside Unit_Test_Chronomodel.app"

Unit_Test_Chronomodel.app/Contents/MacOS/Unit_Test_Chronomodel

echo "Come back to the beging repertory"
cd..