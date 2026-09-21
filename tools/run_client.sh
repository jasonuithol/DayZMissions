#!/bin/bash
# Launch the DayZ Exp client under Proton - vanilla, no mods - skipping the launcher,
# and connect straight to a server. Steam must be running and logged in.
#   tools/run_client.sh                      -> local server
#   tools/run_client.sh 192.168.1.20         -> another machine, default ports
#   tools/run_client.sh 192.168.1.20:2302:27016
#   PROTON=GE-Proton10-32 tools/run_client.sh   -> pick a specific Proton build
STEAM="${STEAM:-$HOME/.steam/debian-installation}"
GAME="$STEAM/steamapps/common/DayZ Exp"
APPID=1024020

SERVER="${1:-127.0.0.1}"
case "$SERVER" in
	*:*) ;;
	*) SERVER="$SERVER:2302:27016" ;;   # ip:gameport:queryport
esac

# newest installed GE-Proton unless told otherwise
if [ -z "$PROTON" ]; then
	PROTON="$(ls "$STEAM/compatibilitytools.d" 2>/dev/null | grep -i proton | sort -V | tail -n1)"
fi
PROTON_BIN="$STEAM/compatibilitytools.d/$PROTON/proton"

if ! pgrep -x steam >/dev/null; then
	echo "Steam isn't running - start it and log in first." >&2
	exit 1
fi
for f in "$GAME/DayZ_BE.exe" "$PROTON_BIN"; do
	if [ ! -e "$f" ]; then
		echo "not found: $f" >&2
		exit 1
	fi
done

export STEAM_COMPAT_CLIENT_INSTALL_PATH="$STEAM"
export STEAM_COMPAT_DATA_PATH="$STEAM/steamapps/compatdata/$APPID"
export STEAM_COMPAT_INSTALL_PATH="$GAME"
export STEAM_COMPAT_APP_ID=$APPID
export SteamAppId=$APPID SteamGameId=$APPID
export PROTON_BATTLEYE_RUNTIME="$STEAM/steamapps/common/Proton BattlEye Runtime"

echo "connecting to $SERVER with $PROTON"
cd "$GAME" || exit 1
# Run Proton on the host: the sniper container (bwrap) is blocked by AppArmor outside of Steam.
exec "$PROTON_BIN" waitforexitandrun \
	"$GAME/DayZ_BE.exe" 0 1 1 -exe DayZ_x64.exe "-connect=$SERVER"
