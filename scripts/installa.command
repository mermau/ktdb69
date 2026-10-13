#!/bin/bash
# Installa DB-69 Dub Filter nelle cartelle dei plugin dell'utente e rimuove la quarantena
cd "$(dirname "$0")"
set -e
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"
rm -rf "$VST3/DB-69 Dub Filter.vst3" "$AU/DB-69 Dub Filter.component"
cp -R "DB-69 Dub Filter.vst3" "$VST3/"
cp -R "DB-69 Dub Filter.component" "$AU/"
rm -rf "/Applications/DB-69 Dub Filter.app" 2>/dev/null || true
if ! cp -R "DB-69 Dub Filter.app" "/Applications/" 2>/dev/null; then
  mkdir -p "$HOME/Applications"; rm -rf "$HOME/Applications/DB-69 Dub Filter.app"; cp -R "DB-69 Dub Filter.app" "$HOME/Applications/"
fi
xattr -dr com.apple.quarantine "$VST3/DB-69 Dub Filter.vst3" "$AU/DB-69 Dub Filter.component" 2>/dev/null || true
xattr -dr com.apple.quarantine "/Applications/DB-69 Dub Filter.app" "$HOME/Applications/DB-69 Dub Filter.app" 2>/dev/null || true
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo ""
echo "DB-69 Dub Filter installato:"
echo "  VST3: $VST3"
echo "  AU:   $AU"
echo "Riavvia la DAW per vederlo."
