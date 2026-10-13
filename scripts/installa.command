#!/bin/bash
# Installa KTDB-69 nelle cartelle dei plugin dell'utente e rimuove la quarantena
cd "$(dirname "$0")"
set -e
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"
rm -rf "$VST3/KTDB-69.vst3" "$AU/KTDB-69.component"
cp -R "KTDB-69.vst3" "$VST3/"
cp -R "KTDB-69.component" "$AU/"
rm -rf "/Applications/KTDB-69.app" 2>/dev/null || true
if ! cp -R "KTDB-69.app" "/Applications/" 2>/dev/null; then
  mkdir -p "$HOME/Applications"; rm -rf "$HOME/Applications/KTDB-69.app"; cp -R "KTDB-69.app" "$HOME/Applications/"
fi
xattr -dr com.apple.quarantine "$VST3/KTDB-69.vst3" "$AU/KTDB-69.component" 2>/dev/null || true
xattr -dr com.apple.quarantine "/Applications/KTDB-69.app" "$HOME/Applications/KTDB-69.app" 2>/dev/null || true
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo ""
echo "KTDB-69 installato:"
echo "  VST3: $VST3"
echo "  AU:   $AU"
echo "Riavvia la DAW per vederlo."
