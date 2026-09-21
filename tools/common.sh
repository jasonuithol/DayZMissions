# Shared settings for the DayZMissions tools. Sourced, not executed.
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SERVER_DIR="${SERVER_DIR:-$HOME/.steam/steam/steamapps/common/DayZ Server Exp}"

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
}
