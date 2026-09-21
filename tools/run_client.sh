#!/bin/bash
# Launch the DayZ client that matches a mission (stable or experimental, with the
# mission's mods) under Proton, skipping the launcher, and connect straight to a server.
# Steam must be running and logged in.
#   tools/run_client.sh coastbikes                 -> local server
#   tools/run_client.sh coastbikes 192.168.1.20    -> another machine, default ports
#   tools/run_client.sh coastbikes 192.168.1.20:2302:27016
#   PROTON=GE-Proton10-32 tools/run_client.sh ...  -> pick a specific Proton build
source "$(dirname "$0")/common.sh"
resolve_mission "$1"

SERVER="${2:-127.0.0.1}"
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
for f in "$CLIENT_DIR/DayZ_BE.exe" "$PROTON_BIN"; do
	if [ ! -e "$f" ]; then
		echo "not found: $f" >&2
		exit 1
	fi
done

setup_mods "$CLIENT_DIR" 0

export STEAM_COMPAT_CLIENT_INSTALL_PATH="$STEAM"
export STEAM_COMPAT_DATA_PATH="$STEAM/steamapps/compatdata/$CLIENT_APPID"
export STEAM_COMPAT_INSTALL_PATH="$CLIENT_DIR"
export STEAM_COMPAT_APP_ID=$CLIENT_APPID
export SteamAppId=$CLIENT_APPID SteamGameId=$CLIENT_APPID
export PROTON_BATTLEYE_RUNTIME="$STEAM/steamapps/common/Proton BattlEye Runtime"

echo "connecting to $SERVER with $PROTON ($GAME client) ${MOD_ARGS[*]}"
cd "$CLIENT_DIR" || exit 1
# Run Proton on the host: the sniper container (bwrap) is blocked by AppArmor outside of Steam.
exec "$PROTON_BIN" waitforexitandrun \
	"$CLIENT_DIR/DayZ_BE.exe" 0 1 1 -exe DayZ_x64.exe "-connect=$SERVER" "${MOD_ARGS[@]}"
