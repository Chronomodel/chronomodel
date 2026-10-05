#!/bin/bash
# version du 2026-09-25
# ne pas mettre de blanc autour de =
#
# Met l'installeur produit par QtIFW_script_macOS.sh dans un fichier .dmg,
# avec un LISEZMOI expliquant comment l'ouvrir (pas de certificat Apple payant :
# voir Ch_copy_library_and_signature.sh / QtIFW_script_macOS.sh).
#
#   cd /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel
#   bash QtIFW_script_macOS.sh
#   bash Ch_make_dmg_macOS.sh                     # prend le dernier installeur cree
#   bash Ch_make_dmg_macOS.sh ChronoModel_v3.3.8_Qt6.11.1_macOS14_20260924_Installer
# _________________________

set -euo pipefail
clear

# ---------------------------- A VERIFIER ---------------------------------
SIGN_ID="-"     # signature ad hoc (pas de compte Apple Developer payant)
# -------------------------------------------------------------------------

ROOT_PATH="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_PATH"

echo "➡️  1 - Recherche de l'installeur"
if [ $# -ge 1 ]; then
    # Argument fourni : avec ou sans ".app", chemin relatif ou absolu
    CANDIDATE="$1"
    [ -e "$CANDIDATE" ] || CANDIDATE="$ROOT_PATH/$1"
    if [ -d "$CANDIDATE.app" ]; then
        INSTALLER_PATH="$CANDIDATE.app"
    elif [ -e "$CANDIDATE" ]; then
        INSTALLER_PATH="$CANDIDATE"
    else
        echo "❌ Installeur introuvable : $1"
        exit 1
    fi
else
    # Pas d'argument : on prend le plus recent produit par QtIFW_script_macOS.sh
    INSTALLER_PATH=$(ls -dt "$ROOT_PATH"/ChronoModel_v*_Installer.app "$ROOT_PATH"/ChronoModel_v*_Installer 2>/dev/null | head -1 || true)
    if [ -z "$INSTALLER_PATH" ]; then
        echo "❌ Aucun installeur ChronoModel_v*_Installer trouve dans $ROOT_PATH"
        echo "   Lancer d'abord : bash QtIFW_script_macOS.sh"
        exit 1
    fi
fi
echo "   Installeur : $INSTALLER_PATH"

INSTALLER_NAME=$(basename "$INSTALLER_PATH")
INSTALLER_NAME="${INSTALLER_NAME%.app}"
DMG="$ROOT_PATH/$INSTALLER_NAME.dmg"

echo "➡️  2 - Signature ad hoc de l'installeur"
# Sans cette signature, l'installeur ne se lance pas sur Apple Silicon.
codesign --force --deep --sign "$SIGN_ID" "$INSTALLER_PATH"
codesign --verify --deep --strict --verbose=2 "$INSTALLER_PATH"

echo "➡️  3 - Preparation du contenu du .dmg"
DMG_DIR="$ROOT_PATH/dmg_content"
rm -rf "$DMG_DIR" "$DMG"
mkdir -p "$DMG_DIR"
ditto "$INSTALLER_PATH" "$DMG_DIR/$(basename "$INSTALLER_PATH")"

cat > "$DMG_DIR/LISEZMOI.txt" <<EOF
ChronoModel - installation sur macOS

Cette application n'est pas signee avec un certificat Apple payant : macOS affiche donc
un avertissement a la premiere ouverture (« Apple ne peut pas verifier... »). Ce n'est pas
le signe d'un probleme, il suffit d'autoriser l'installeur une seule fois :

1. Copiez l'installeur du disque vers votre Bureau ou vos Documents.
2. Double-cliquez dessus : macOS refuse de l'ouvrir. Cliquez sur « Termine » (ou « OK »).
3. Ouvrez Reglages Systeme > Confidentialite et securite, descendez jusqu'a la section
   « Securite » et cliquez sur « Ouvrir quand meme » a cote du nom de l'installeur.
   Confirmez avec votre mot de passe, puis cliquez sur « Ouvrir ».

Alternative dans le Terminal (adapter le chemin) :
   xattr -dr com.apple.quarantine "/chemin/vers/$(basename "$INSTALLER_PATH")"

Sur macOS 14 et anterieur, clic droit sur l'installeur > Ouvrir suffit generalement.

L'application installee ensuite s'ouvre normalement ; si macOS la refuse aussi, utilisez
la meme procedure (Confidentialite et securite > Ouvrir quand meme).
EOF

echo "➡️  4 - Creation du .dmg"
hdiutil create -volname "$INSTALLER_NAME" -srcfolder "$DMG_DIR" -ov -format UDZO "$DMG"
rm -rf "$DMG_DIR"

echo "➡️  5 - Verification"
hdiutil verify "$DMG"

echo "✅ Termine : $DMG"