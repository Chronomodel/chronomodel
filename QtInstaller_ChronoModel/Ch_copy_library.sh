#!/bin/bash
# ne pas mettre de blanc autour de =
#
# pour lancer
# cd /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel
# sh Ch_copy_library.sh
#_____________________________________

clear
# _________________________
echo "$  1 Script copie qt librairie "
# -------------------------------------------------------

# -------------------------------------------------------
#	Vérifier que le chemin de Qt est bien celui de la machine; mettre le numero de version
# -------------------------------------------------------
ROOT_PATH=$(dirname $0)

RELEASE_PATH=/Users/dufresne/ChronoModel-SoftWare/build-Chronomodel-Qt_6_5_3_for_macOS-Release/build/release/
BUNDLE="$RELEASE_PATH"chronomodel.app
echo "copie dans le BUNDLE $BUNDLE"

QT_BIN_PATH=/Users/dufresne/Qt/6.5.3/macos/bin
QT_LIB_PATH=/Users/dufresne/Qt/6.5.3/macos/lib
QT_PLUGINS_PATH=/Users/dufresne/Qt/6.5.3/macos/plugins
VERSION=3.2.7


# le texte suivant est remplacé par macdeployqt
# -------------------------------------------------------
#  Copier les librairies Qt dans bundle
#  dans Contents/Frameworks:
# 		Contents/Frameworks/QtCore.framework
#		Contents/Frameworks/QtDBus.framework
#		Contents/Frameworks/QtDBus.framework
#		Contents/Frameworks/QtSvg.framework
#		Contents/Frameworks/QtWidgets.framework
#
# L'option -L après cp suit le liens symbolic
# -------------------------------------------------------

#mkdir "$BUNDLE"/Contents/Frameworks
#mkdir "$BUNDLE"/Contents/Frameworks/QtCore.framework
#cp -R -L "$QT_LIB_PATH"/QtCore.framework/QtCore "$BUNDLE"/Contents/Frameworks/QtCore.framework/QtCore

#mkdir "$BUNDLE"/Contents/Frameworks/QtDBus.framework
#cp -R -L "$QT_LIB_PATH"/QtDBus.framework/QtDBus "$BUNDLE"/Contents/Frameworks/QtDBus.framework/QtDBus

#mkdir "$BUNDLE"/Contents/Frameworks/QtDBus.framework
#cp -R -L "$QT_LIB_PATH"/QtDBus.framework/QtDBus "$BUNDLE"/Contents/Frameworks/QtDBus.framework/QtDBus

#mkdir "$BUNDLE"/Contents/Frameworks/QtSvg.framework
#cp -R -L "$QT_LIB_PATH"/QtSvg.framework/QtSvg "$BUNDLE"/Contents/Frameworks/QtSvg.framework/QtSvg

#mkdir "$BUNDLE"/Contents/Frameworks/QtWidgets.framework
#cp -R -L "$QT_LIB_PATH"/QtWidgets.framework/QtWidgets "$BUNDLE"/Contents/Frameworks/QtWidgets.framework/QtWidgets

# -------------------------------------------------------
#  dans Contents/PlugIns:
#		Contents/PlugIns/iconengines
#		Contents/PlugIns/imageformats
#		Contents/PlugIns/platforms
#		Contents/PlugIns/styles
# -------------------------------------------------------
#mkdir "$BUNDLE"/Contents/PlugIns
#mkdir "$BUNDLE"/Contents/PlugIns/iconengines
#cp -R "$QT_PLUGINS_PATH"/iconengines/*.dylib "$BUNDLE"/Contents/PlugIns/iconengines/

#mkdir "$BUNDLE"/Contents/PlugIns/imageformats
#cp -R "$QT_PLUGINS_PATH"/imageformats/*.dylib "$BUNDLE"/Contents/PlugIns/imageformats/

#mkdir "$BUNDLE"/Contents/PlugIns/platforms
#cp -R "$QT_PLUGINS_PATH"/platforms/*.dylib "$BUNDLE"/Contents/PlugIns/platforms/

#mkdir "$BUNDLE"/Contents/PlugIns/styles
#cp -R "$QT_PLUGINS_PATH"/styles/*.dylib "$BUNDLE"/Contents/PlugIns/styles/


echo "$ 2 Execution de macdeployqt"
${QT_BIN_PATH}/macdeployqt $BUNDLE


# Le Finder ne détecte généralement pas immédiatement le changement d'icône.
# Copiez le paquet dans un autre dossier pour qu’il enregistre la nouvelle icône

echo "$ 3 Insertion de la version dans Info.plist "
# https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html

PLIST=${BUNDLE}/Contents/Info.plist
/usr/libexec/Plistbuddy -c "Set :CFBundleIdentifier fr.CNRS.chronomodel" "$PLIST"
/usr/libexec/Plistbuddy -c "Set :CFBundleSignature chml" "$PLIST"
/usr/libexec/Plistbuddy -c "Set :CFBundleExecutable chronomodel" "$PLIST"

/usr/libexec/Plistbuddy -c "Add :CFBundleVersion string ${VERSION}" "$PLIST"
/usr/libexec/Plistbuddy -c "Add :CFBundleShortVersionString string ${VERSION}" "$PLIST"

/usr/libexec/Plistbuddy -c "Add :CFBundleIconFile string Chronomodel.icns" "$PLIST"

/usr/libexec/Plistbuddy -c "Add CFBundleDocumentTypes array" "$PLIST"
/usr/libexec/Plistbuddy -c "Add :CFBundleDocumentTypes:0:CFBundleTypeRole string Editor" "$PLIST"
/usr/libexec/Plistbuddy -c "Add :CFBundleDocumentTypes:0:CFBundleTypeIconFile string Chronomodel.icns" "$PLIST"
/usr/libexec/Plistbuddy -c "Add :CFBundleDocumentTypes:0:CFBundleTypeName string Chronomodel Project" "$PLIST"

echo "$ 0.4.1 $PLIST final"
/usr/libexec/PlistBuddy -x -c "Print" "$PLIST"
