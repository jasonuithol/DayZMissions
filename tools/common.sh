# Shared settings for the DayZMissions tools. Sourced, not executed.
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STEAM="${STEAM:-$HOME/.steam/debian-installation}"

# Which DayZ a mission runs on comes from GAME=exp|stable in its mission.conf (default exp).
select_game() {
	case "$1" in
		exp)
			SERVER_DIR="$STEAM/steamapps/common/DayZ Server Exp"
			CLIENT_DIR="$STEAM/steamapps/common/DayZ Exp"
			CLIENT_APPID=1024020 ;;
		stable)
			SERVER_DIR="$STEAM/steamapps/common/DayZServer"
			CLIENT_DIR="$STEAM/steamapps/common/DayZ"
			CLIENT_APPID=221100 ;;
		*)
			echo "unknown GAME '$1' (expected exp or stable)" >&2
			exit 1 ;;
	esac
}

# Missions live in missions/<name>.<terrain>/ e.g. missions/coastbikes.chernarusplus
resolve_mission() {
	local name="$1"
	if [ -z "$name" ]; then
		echo "usage: $(basename "$0") <mission>" >&2
		echo "available missions:" >&2
		ls "$PROJECT_DIR/missions" >&2
		exit 1
	fi
	# allow "coastbikes" as shorthand for "coastbikes.chernarusplus"
	if [ ! -d "$PROJECT_DIR/missions/$name" ]; then
		local match
		match=$(ls -d "$PROJECT_DIR/missions/$name".* 2>/dev/null | head -n1)
		[ -n "$match" ] && name="$(basename "$match")"
	fi
	if [ ! -d "$PROJECT_DIR/missions/$name" ]; then
		echo "no such mission: $name" >&2
		exit 1
	fi
	MISSION="$name"
	TERRAIN="${name##*.}"

	GAME=exp
	[ -f "$PROJECT_DIR/missions/$MISSION/mission.conf" ] && source "$PROJECT_DIR/missions/$MISSION/mission.conf"
	select_game "$GAME"
}

WORKSHOP_DIR="${WORKSHOP_DIR:-$STEAM/steamapps/workshop/content/221100}"

# Reads missions/<mission>/mods.txt (lines of "<workshop id> <@Name>", in load order;
# "local <@Name>" is one of our own mods, built from mods/<Name> into build/),
# links each mod into game_dir as @Name and sets MOD_ARGS to the -mod= argument.
# @Name is a real folder of symlinks with lower-cased names, because some mods ship
# "Addons"/"Keys" and the Linux server only looks for "addons".
# With install_keys=1 the mods' .bikey files are copied into game_dir/keys (server only).
# NOMODS=1 skips everything, for running a mission vanilla.
setup_mods() {
	local game_dir="$1" install_keys="$2"
	local list="$PROJECT_DIR/missions/$MISSION/mods.txt"
	local mods="" id name entry
	MOD_ARGS=()
	[ -n "$NOMODS" ] && return 0
	[ -f "$list" ] || return 0

	while read -r id name; do
		case "$id" in ""|\#*) continue ;; esac
		local src="$WORKSHOP_DIR/$id"
		if [ "$id" = local ]; then
			"$PROJECT_DIR/tools/build_mod.py" "$PROJECT_DIR/mods/${name#@}" "$PROJECT_DIR/build" >/dev/null || exit 1
			src="$PROJECT_DIR/build/$name"
		elif [ ! -d "$src" ]; then
			echo "mod $name ($id) is not downloaded - subscribe to it in Steam" >&2
			exit 1
		fi
		[ -L "$game_dir/$name" ] && rm "$game_dir/$name"
		mkdir -p "$game_dir/$name"
		for entry in "$src"/*; do
			ln -sfn "$entry" "$game_dir/$name/$(basename "$entry" | tr '[:upper:]' '[:lower:]')"
		done
		if [ "$install_keys" = 1 ]; then
			find "$src/" -iname '*.bikey' -exec cp -u {} "$game_dir/keys/" ';'
		fi
		mods="$mods${mods:+;}$name"
	done < "$list"

	[ -n "$mods" ] && MOD_ARGS=("-mod=$mods")
	return 0
}
