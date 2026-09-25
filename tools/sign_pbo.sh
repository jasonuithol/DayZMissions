#!/bin/bash
# Sign a PBO with DayZ Tools' DSSignFile, run under Proton's Wine (no Windows needed).
#   tools/sign_pbo.sh <key name> <file.pbo>     -> <file.pbo>.<key>.bisign next to it
# The private key lives in ~/.dayz-keys/<key>.biprivatekey (never in the repo); the
# matching <key>.bikey goes into the mod's keys/ folder and the server's keys/.
# Make a key pair once with:  tools/sign_pbo.sh --create <key name>
set -e
STEAM="${STEAM:-$HOME/.steam/debian-installation}"
DSUTILS="$STEAM/steamapps/common/DayZ Tools/Bin/DsUtils"
PROTON="$(ls -d "$STEAM"/compatibilitytools.d/GE-Proton* 2>/dev/null | sort -V | tail -n1)"
KEYS="$HOME/.dayz-keys"
export WINEPREFIX="$KEYS/wineprefix" WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml="
WINE="$PROTON/files/bin/wine64"
[ -x "$WINE" ] || { echo "no GE-Proton found under $STEAM/compatibilitytools.d" >&2; exit 1; }
mkdir -p "$KEYS"

if [ "$1" = --create ]; then
	cd "$KEYS" && "$WINE" "$DSUTILS/DSCreateKey.exe" "$2" 2>/dev/null
	chmod 600 "$KEYS/$2.biprivatekey"
	echo "created $KEYS/$2.bikey and $2.biprivatekey"
	exit 0
fi

KEY="$1"; PBO="$(realpath "$2")"
[ -f "$KEYS/$KEY.biprivatekey" ] || { echo "no private key $KEYS/$KEY.biprivatekey (tools/sign_pbo.sh --create $KEY)" >&2; exit 1; }
cd "$(dirname "$PBO")"
rm -f "$(basename "$PBO")".*.bisign
# Wine wants Windows paths for arguments: Z: is the Linux root
winpath() { echo "Z:${1//\//\\}"; }
"$WINE" "$DSUTILS/DSSignFile.exe" "$(winpath "$KEYS/$KEY.biprivatekey")" "$(basename "$PBO")" 2>/dev/null
[ -f "$(basename "$PBO").$KEY.bisign" ] || { echo "signing failed for $PBO" >&2; exit 1; }
