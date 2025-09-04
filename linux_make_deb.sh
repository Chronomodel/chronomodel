#!/bin/bash
# bash linux_make_deb.sh
# Script pour construire un package Debian (Debian standard avec changelog)
# Auteur: Philippe Dufresne

set -euo pipefail

# Variables
SOURCES_PATH=~/chronomodel/chronomodel 
DATE_FILE=$(date '+%Y%m%d')
VERSION=3.3.5
PACKAGE_NAME=chronomodel_v${VERSION}_Qt6.4.2_amd64_deb12_${DATE_FILE}

DEPLOY_PATH=$SOURCES_PATH/deploy/linux
WORKING_PATH=~/chronomodel/$PACKAGE_NAME # doit être en lowercase
MAINTAINER_NAME="Dufresne Philippe"
MAINTAINER_EMAIL="philippe.dufresne@univ-rennes.fr"
CHANGES_MESSAGE="Package build ${VERSION} for Qt6.4.2; automated build on ${DATE_FILE}."

echo " 1 - RAZ du dossier de travail: $WORKING_PATH"
sudo rm -rf "$WORKING_PATH"

echo " 2 - Construction de la structure"
mkdir -p "$WORKING_PATH/DEBIAN"
mkdir -p "$WORKING_PATH/usr/bin"
mkdir -p "$WORKING_PATH/usr/share/icons/hicolor/scalable/apps"
mkdir -p "$WORKING_PATH/usr/share/doc/chronomodel/"
mkdir -p "$WORKING_PATH/usr/share/applications/"
mkdir -p "$WORKING_PATH/usr/share/man/man1/"
tree "$WORKING_PATH" || true

echo " 3 - Copie de l'application dans /usr/bin"
cp "$SOURCES_PATH/build/release/chronomodel" "$WORKING_PATH/usr/bin/chronomodel"
strip "$WORKING_PATH/usr/bin/chronomodel"
sudo chmod -R 755 "$WORKING_PATH/usr"

echo " 4 - Copie des icones"
cp "$SOURCES_PATH/icon/Chronomodel.png" "$WORKING_PATH/usr/share/icons/chronomodel.png"
sudo chmod 0644 "$WORKING_PATH/usr/share/icons/chronomodel.png"
cp "$SOURCES_PATH/icon/Chronomodel.png" "$WORKING_PATH/usr/share/icons/hicolor/chronomodel.png"
sudo chmod 0644 "$WORKING_PATH/usr/share/icons/hicolor/chronomodel.png"
cp "$SOURCES_PATH/icon/icon_svg.svg" "$WORKING_PATH/usr/share/icons/hicolor/scalable/apps/chronomodel.svg"
sudo chmod 0644 "$WORKING_PATH/usr/share/icons/hicolor/scalable/apps/chronomodel.svg"

echo " 5 - Préparation et mise à jour du changelog Debian"
cd "$SOURCES_PATH"
mkdir -p debian

# On copie ton changelog historique comme base
if [ -f "$DEPLOY_PATH/changelog" ]; then
    cp "$DEPLOY_PATH/changelog" debian/changelog
fi

if command -v dch >/dev/null 2>&1; then
    if [ ! -f debian/changelog ]; then
        dch --create -v "${VERSION}-1" --package chronomodel "${CHANGES_MESSAGE}"
    else
        dch -v "${VERSION}-1" --package chronomodel "${CHANGES_MESSAGE}"
    fi
    sed -i '/Closes:/d' debian/changelog
else
    # Ajout manuel si pas de dch
    tmpfile=$(mktemp)
    cat > "$tmpfile" <<EOF
chronomodel (${VERSION}-1) unstable; urgency=medium

  * ${CHANGES_MESSAGE}

 -- ${MAINTAINER_NAME} <${MAINTAINER_EMAIL}>  $(date -R)

EOF
    # Concatène nouvelle entrée + ancien changelog
    cat "$tmpfile" debian/changelog > debian/changelog.new
    mv debian/changelog.new debian/changelog
    rm "$tmpfile"
fi

# Copie vers l’arborescence du paquet
cp debian/changelog "$WORKING_PATH/usr/share/doc/chronomodel/changelog"
gzip -9n "$WORKING_PATH/usr/share/doc/chronomodel/changelog"
sudo chmod 0644 "$WORKING_PATH/usr/share/doc/chronomodel/changelog.gz"

# Fichiers doc
cp "$SOURCES_PATH/README" "$WORKING_PATH/usr/share/doc/chronomodel/README"
sudo chmod 0644 "$WORKING_PATH/usr/share/doc/chronomodel/README"

cp "$DEPLOY_PATH/copyright" "$WORKING_PATH/usr/share/doc/chronomodel/"
sudo chmod 0644 "$WORKING_PATH/usr/share/doc/chronomodel/copyright"

echo " 6 - Copie de la page man"
cp "$DEPLOY_PATH/chronomodel.1" "$WORKING_PATH/usr/share/man/man1/chronomodel.1"
gzip -9n "$WORKING_PATH/usr/share/man/man1/chronomodel.1"
sudo chmod 0644 "$WORKING_PATH/usr/share/man/man1/chronomodel.1.gz"

echo " 7 - Copie du fichier .desktop"
cp "$DEPLOY_PATH/chronomodel.desktop" "$WORKING_PATH/usr/share/applications/"
sudo chmod 0644 "$WORKING_PATH/usr/share/applications/chronomodel.desktop"

echo " 8 - Copie du fichier control dans DEBIAN/"
cp "$DEPLOY_PATH/control" "$WORKING_PATH/DEBIAN/control"

echo " 9 - Structure avant création du package"
tree "$WORKING_PATH" || true

sudo chown -R root:root "$WORKING_PATH"

echo " 10 - Construction du package"
dpkg-deb --build "$WORKING_PATH"

echo " 11 - Contrôle avec lintian"
lintian "$WORKING_PATH/../$PACKAGE_NAME.deb" || true

echo "Pour installer :"
echo "  sudo dpkg --install $PACKAGE_NAME.deb"
echo
echo "Pour désinstaller :"
echo "  sudo dpkg --remove chronomodel"

