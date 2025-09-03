#!/bin/sh
sh create-dmg.sh \
  --volname "ChronModel Installer" \
  --volicon "/Users/dufresne/ChronoModel-SoftWare/chronomodel/icon/Chronomodel.icns" \
  --background "/Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel/Chronomodel.png" \
  --window-pos 200 120 \
  --window-size 800 400 \
  --icon-size 100 \
  --app-drop-link 600 185 \
  "ChronoModel-Installer.dmg" \
  "/Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel/ChronoModel_v3.2.8_installer/"