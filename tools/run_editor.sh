#!/bin/bash
# Launch the stable DayZ client offline with the DayZ Editor (Workshop 2250764298, needs CF
# and Dabs Framework) for browsing every object and laying out builds. Our own mods named on
# the command line are built and loaded too, so their items show up in the editor's list.
# Steam must be running and logged in, and subscribed to the three Workshop items.
#   tools/run_editor.sh                 -> the editor on its own
#   tools/run_editor.sh DayZPortals     -> plus mods/DayZPortals
#   PROTON=GE-Proton10-32 tools/run_editor.sh ...  -> pick a specific Proton build
source "$(dirname "$0")/common.sh"
select_game stable

MODS_LIST="$(mktemp)"
trap 'rm -f "$MODS_LIST"' EXIT
cat > "$MODS_LIST" <<LIST
1559212036 @CF
2545327648 @Dabs-Framework
2250764298 @DayZ-Editor
LIST
for name in "$@"; do
	[ -d "$PROJECT_DIR/mods/$name" ] || { echo "no such mod: mods/$name" >&2; exit 1; }
	echo "local @$name" >> "$MODS_LIST"
done

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

setup_mods "$CLIENT_DIR" 0 dayzmissions-mods

export STEAM_COMPAT_CLIENT_INSTALL_PATH="$STEAM"
export STEAM_COMPAT_DATA_PATH="$STEAM/steamapps/compatdata/$CLIENT_APPID"
export STEAM_COMPAT_INSTALL_PATH="$CLIENT_DIR"
export STEAM_COMPAT_APP_ID=$CLIENT_APPID
export SteamAppId=$CLIENT_APPID SteamGameId=$CLIENT_APPID
export PROTON_BATTLEYE_RUNTIME="$STEAM/steamapps/common/Proton BattlEye Runtime"

echo "starting the DayZ Editor with $PROTON ${MOD_ARGS[*]}"
cd "$CLIENT_DIR" || exit 1
# Run Proton on the host: the sniper container (bwrap) is blocked by AppArmor outside of Steam.
# No exec: the trap has to remove the mod list afterwards.
"$PROTON_BIN" waitforexitandrun \
	"$CLIENT_DIR/DayZ_BE.exe" 0 1 1 -exe DayZ_x64.exe "${MOD_ARGS[@]}"
