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

# Reads missions/<mission>/mods.txt (lines of "<workshop id> <@Name> [server]", in load order;
# "local <@Name>" is one of our own mods, built from mods/<Name> into build/; "server" marks a
# server-side mod, loaded with -serverMod= and left out on clients), links each mod into
# game_dir as @Name and sets MOD_ARGS (and SERVERMOD_ARGS) to the -mod= (-serverMod=) argument.
# @Name is a real folder of symlinks with lower-cased names, because some mods ship
# "Addons"/"Keys" and the Linux server only looks for "addons".
# With install_keys=1 the mods' .bikey files are copied into game_dir/keys (server only).
# With subdir set, the links go into game_dir/<subdir>/ instead of the game root: the
# DayZ launcher scans the client's root for @* folders and treats them as local mods, which
# breaks its Workshop matching ("unrecognised" mods), so the client uses a subfolder.
# NOMODS=1 skips everything, for running a mission vanilla.
setup_mods() {
	local game_dir="$1" install_keys="$2" subdir="$3"
	local link_dir="$game_dir${subdir:+/$subdir}"
	mkdir -p "$link_dir"
	local list="$PROJECT_DIR/missions/$MISSION/mods.txt"
	local mods="" servermods="" id name kind entry
	MOD_ARGS=()
	SERVERMOD_ARGS=()
	[ -n "$NOMODS" ] && return 0
	[ -f "$list" ] || return 0

	while read -r id name kind; do
		case "$id" in ""|\#*) continue ;; esac
		# server-side mods: only on the server (install_keys=1 is the server)
		[ "$kind" = server ] && [ "$install_keys" != 1 ] && continue
		local src="$WORKSHOP_DIR/$id"
		if [ "$id" = local ]; then
			# NO_BUILD (the VPS): use the signed build shipped from the dev machine as is
			[ -n "$NO_BUILD" ] || "$PROJECT_DIR/tools/build_mod.py" "$PROJECT_DIR/mods/${name#@}" "$PROJECT_DIR/build" >/dev/null || exit 1
			src="$PROJECT_DIR/build/$name"
		elif [ ! -d "$src" ]; then
			echo "mod $name ($id) is not downloaded - subscribe to it in Steam" >&2
			exit 1
		fi
		[ -L "$link_dir/$name" ] && rm "$link_dir/$name"
		mkdir -p "$link_dir/$name"
		for entry in "$src"/*; do
			ln -sfn "$entry" "$link_dir/$name/$(basename "$entry" | tr '[:upper:]' '[:lower:]')"
		done
		if [ "$install_keys" = 1 ]; then
			find "$src/" -iname '*.bikey' -exec cp -u {} "$game_dir/keys/" ';'
		fi
		if [ "$kind" = server ]; then
			servermods="$servermods${servermods:+;}$name"
		else
			mods="$mods${mods:+;}${subdir:+$subdir/}$name"
		fi
	done < "$list"

	[ -n "$mods" ] && MOD_ARGS=("-mod=$mods")
	[ -n "$servermods" ] && SERVERMOD_ARGS=("-serverMod=$servermods")
	return 0
}
