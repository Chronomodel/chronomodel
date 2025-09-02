#!/bin/bash
# bash linux_make_deb.sh
# https://blog.packagecloud.io/eng/2016/12/15/howto-build-debian-package-containing-simple-shell-scripts/
# https://blog.packagecloud.io/debian/debuild/packaging/2015/06/08/buildling-deb-packages-with-debuild/
reset

# instal qt creator first https://www.qt.io/download#section-2
#sudo apt-get install devscripts build-essential lintian
#sudo apt install dh-make 
#sudo apt-get install qt6-default

# https://kristuff.fr/blog/post/create-debian-package-a-practical-guide

SOURCES_PATH=~/chronomodel/chronomodel 

cd $SOURCES_PATH
DATE_FILE=$(date '+%Y%m%d')
VERSION=3.3.5
PACKAGE_NAME=chronomodel_v${VERSION}_Qt6.4.2_amd64_deb12_${DATE_FILE}

DEPLOY_PATH=$SOURCES_PATH/deploy/linux
WORKING_PATH=~/chronomodel/$PACKAGE_NAME # this directoy must be in lowercase

echo " 1 - RAZ du dossier de travail" $WORKING_PATH
sudo rm -fr $WORKING_PATH

echo " 2 - construction de la structure"
mkdir $WORKING_PATH 
mkdir $WORKING_PATH/DEBIAN
mkdir -p $WORKING_PATH/usr/bin
mkdir -p $WORKING_PATH/usr/share/icons/hicolor/scalable/apps
mkdir -p $WORKING_PATH/usr/share/doc/chronomodel/
mkdir -p $WORKING_PATH/usr/share/applications/
mkdir -p $WORKING_PATH/usr/share/man/man1/

# test existance de tree, sinon installation
#if dpkg -s "tree" &> /dev/null; then
#    echo "tree est déjà installé."
#else
#    echo "tree n'est pas installé. Installation en cours..."
#    sudo apt update
#    sudo apt install "tree"
#fi

tree $WORKING_PATH 

# we must rename the application, because the command must be in lowercase
echo " 3 - Copie de l'application dans /usr/bin "

#cp $SOURCES_PATH/build/release/chronomodel $WORKING_PATH/usr/bin/chronomodel
cp $SOURCES_PATH/build/release/chronomodel $WORKING_PATH/usr/bin/chronomodel
# strip protége les données !
strip $WORKING_PATH/usr/bin/chronomodel
sudo chmod -R 755 $WORKING_PATH/usr

#cp $DEPLOY_PATH/../ABOUT.html $WORKING_PATH/usr/bin/ABOUT.html

echo " 4 - Copie des icones dans usr/share/icons"
cp $SOURCES_PATH/icon/Chronomodel.png $WORKING_PATH/usr/share/icons/chronomodel.png
sudo chmod 0644 $WORKING_PATH/usr/share/icons/chronomodel.png
cp $SOURCES_PATH/icon/Chronomodel.png $WORKING_PATH/usr/share/icons/hicolor/chronomodel.png
sudo chmod 0644 $WORKING_PATH/usr/share/icons/hicolor/chronomodel.png
cp $SOURCES_PATH/icon/icon_svg.svg $WORKING_PATH/usr/share/icons/hicolor/scalable/apps/chronomodel.svg
sudo chmod 0644 $WORKING_PATH/usr/share/icons/hicolor/scalable/apps/chronomodel.svg


echo " 5 - Copie des fichiers dans /usr/share/doc: changelog, changelog.debian, readme, copyright, chronomodel.1"
#cd $WORKING_PATH
#dch -i  #--Create a debian/changelog file
# gzip -9n ; 9 pour la meilleur compression --best, et n pour enlever l'horodatage
# copie en double pour la compatibilité du systeme
cp $DEPLOY_PATH/changelog $WORKING_PATH/usr/share/doc/chronomodel/changelog
gzip -9n $WORKING_PATH/usr/share/doc/chronomodel/changelog
sudo chmod 0644 $WORKING_PATH/usr/share/doc/chronomodel/changelog.gz

cp $SOURCES_PATH/README $WORKING_PATH/usr/share/doc/chronomodel/README
sudo chmod 0644 $WORKING_PATH/usr/share/doc/chronomodel/README

cp $DEPLOY_PATH/copyright $WORKING_PATH/usr/share/doc/chronomodel/.
sudo chmod 0644 $WORKING_PATH/usr/share/doc/chronomodel/copyright

echo " 6 - Copie de la page manuel , compression et supprime le timestamp"
cp $DEPLOY_PATH/chronomodel.1 $WORKING_PATH/usr/share/man/man1/chronomodel.1
gzip -9n $WORKING_PATH/usr/share/man/man1/chronomodel.1
sudo chmod 0644 $WORKING_PATH/usr/share/man/man1/chronomodel.1.gz

cp $DEPLOY_PATH/chronomodel.desktop $WORKING_PATH/usr/share/applications/.
sudo chmod 0644 $WORKING_PATH/usr/share/applications/chronomodel.desktop

# sudo chmod -R drwxr-xr-r $WORKING_PATH/usr/share

echo " 7 - modification fichier changelog.gz"
sed -i 's/Tus, 26 Sep 2024 17:09:05 +0200/Tue, 26 Sep 2024 17:09:05 +0200/' $WORKING_PATH/usr/share/doc/chronomodel/changelog.gz


echo " 8 - Copie des fichiers dans dossier DEBIAN: control, copyright"
cp $DEPLOY_PATH/control $WORKING_PATH/DEBIAN/control

echo " 9 - Structure avant création du package"
tree $WORKING_PATH 


sudo chown -R root:root  $WORKING_PATH

dpkg-deb --build $WORKING_PATH 

echo " - 10 Controle avec Execution lintian"
lintian $WORKING_PATH/../$PACKAGE_NAME.deb

#ar -rc CM327.deb debian-binary control.tar.xz data.tar.xz

echo Pour installer en ligne de commande
echo sudo dpkg --install $PACKAGE_NAME.deb
echo
echo Pour desinstaller
echo dpkg --remove chronomodel
# Pour forcer les dépendances "-y"
# sudo dpkg -i --force-depends chronomodel_3.2.8_amd64.deb

# sudo apt ou apt-gat avec -y gere les dépendances manquantes
# sudo apt install -y chronomodel_3.2.8_amd64.deb
# sudo apt-get install -y chronomodel_3.2.8_amd64.deb


# Pour désinstaller en ligne de commande
# dpkg --remove chronomodel

