#!/bin/bash
# Version du 2025-08-28
# Script : Ch_copy_library.sh
# Chemin : /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel
#
# Pour lancer :
#   cd /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel
#   sh Ch_copy_library.sh
#
# Note : ne pas mettre de blanc autour de "="
# ________________________________________________________

clear

echo "➡️  [1] Lancement du script de copie des librairies Qt"

# -------------------------------------------------------
# Paramètres de chemins (à adapter selon la machine)
# -------------------------------------------------------
ROOT_PATH=$(dirname $0)
RELEASE_PATH=/Users/dufresne/ChronoModel-SoftWare/chronomodel/build/Qt_6_10_0_for_macOS-Release/build/release/
BUNDLE="${RELEASE_PATH}chronomodel.app"

QT_BIN_PATH=/Users/dufresne/Qt/6.10.0/macos/bin
QT_LIB_PATH=/Users/dufresne/Qt/6.10.0/macos/lib
QT_PLUGINS_PATH=/Users/dufresne/Qt/6.10.0/macos/plugins
VERSION=3.3.5

# -------------------------------------------------------
# Copie manuelle des frameworks et plugins Qt
# (désactivée car remplacée par macdeployqt)
# -------------------------------------------------------
# Exemple pour un framework :
#   mkdir "$BUNDLE"/Contents/Frameworks/QtCore.framework
#   cp -R -L "$QT_LIB_PATH"/QtCore.framework/QtCore "$BUNDLE"/Contents/Frameworks/QtCore.framework/QtCore
#
# Exemple pour un plugin :
#   mkdir "$BUNDLE"/Contents/PlugIns/platforms
#   cp -R "$QT_PLUGINS_PATH"/platforms/*.dylib "$BUNDLE"/Contents/PlugIns/platforms/


# -------------------------------------------------------
# Étape 2 : Mise à jour de Info.plist
# -------------------------------------------------------
echo "➡️  [2] Mise à jour de Info.plist"
PLIST=${BUNDLE}/Contents/Info.plist

# Identifiant unique de l’app -- ne pas modifier
# Memo: Modifier CFBundleIdentifier après macdeployqt change le “bundle identity”.
#macOS considère l’app comme une nouvelle app.
#Tous les mécanismes d’ouverture de fichiers automatiques (double-clic, fichiers associés)
# sont cassés pour les fichiers non déclarés dans Info.plist.
#/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier fr.cnrs.chronomodel" "$PLIST"

# Versions
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString ${VERSION}" "$PLIST"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion ${VERSION}" "$PLIST"

# Nettoyage au cas où
/usr/libexec/PlistBuddy -c "Delete :CFBundleDocumentTypes" "$PLIST" 2>/dev/null
/usr/libexec/PlistBuddy -c "Delete :UTExportedTypeDeclarations" "$PLIST" 2>/dev/null

# Déclaration du type de document (.chr)
usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes array" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:0 dict" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:0:CFBundleTypeName string 'ChronoModel Project'" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:0:CFBundleTypeRole string Editor" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:0:CFBundleTypeIconFile string Chronomodel.icns" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:0:LSItemContentTypes array" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:0:LSItemContentTypes:0 string fr.cnrs.chronomodel.project" "$PLIST"

# Déclaration du type UTI (.chr)
usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations array" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations:0 dict" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations:0:UTTypeIdentifier string fr.cnrs.chronomodel.project" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations:0:UTTypeDescription string 'ChronoModel Project File'" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations:0:UTTypeConformsTo array" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations:0:UTTypeConformsTo:0 string public.data" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification dict" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification:public.filename-extension array" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification:public.filename-extension:0 string chr" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification:public.mime-type string application/x-chronomodel" "$PLIST"

# -------------------------------------------------------
# Vérification finale
# -------------------------------------------------------
# echo "✅  [4] Info.plist mis à jour avec succès"
# /usr/libexec/PlistBuddy -c "Print" "$PLIST"
# -------------------------------------------------------
# Étape 3 : Utilisation de macdeployqt pour déploiement auto
# -------------------------------------------------------
echo "➡️  [3] Exécution de macdeployqt"
${QT_BIN_PATH}/macdeployqt $BUNDLE

echo "✅"
