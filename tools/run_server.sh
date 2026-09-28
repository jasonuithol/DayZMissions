#!/bin/bash
# Deploy a mission and start the server on it, with the mods from its mods.txt (if any).
#   NOMODS=1 tools/run_server.sh coastbikes   -> vanilla
#   tools/run_server.sh coastbikes
set -e
source "$(dirname "$0")/common.sh"
resolve_mission "$1"
shift

"$PROJECT_DIR/tools/deploy.sh" "$MISSION"

setup_mods "$SERVER_DIR" 1

cd "$SERVER_DIR"

# VERIFY_SIGNATURES in mission.conf (needed for our own unsigned mods), or any of
# SERVER_NAME / SERVER_PASSWORD / ADMIN_PASSWORD / QUERY_PORT / MOTD in the environment
# (the VPS), get the mission its own copy of the server config; serverDZ.cfg is left
# alone. MOTD is one or more lines separated by "|", shown in turn every 5 minutes.
CONFIG=serverDZ.cfg
if [ -n "$VERIFY_SIGNATURES$SERVER_NAME$SERVER_PASSWORD$ADMIN_PASSWORD$QUERY_PORT$MOTD" ]; then
	CONFIG="serverDZ.${MISSION%%.*}.cfg"
	sed -E \
		-e "s/^verifySignatures *= *[0-9]+;/verifySignatures = ${VERIFY_SIGNATURES:-2};/" \
		${SERVER_NAME:+-e "s/^hostname *= *\"[^\"]*\";/hostname = \"$SERVER_NAME\";/"} \
		${SERVER_PASSWORD+-e "s/^password *= *\"[^\"]*\";/password = \"$SERVER_PASSWORD\";/"} \
		${ADMIN_PASSWORD:+-e "s/^passwordAdmin *= *\"[^\"]*\";/passwordAdmin = \"$ADMIN_PASSWORD\";/"} \
		serverDZ.cfg > "$CONFIG"
	grep -q "^steamQueryPort" "$CONFIG" || printf '\nsteamQueryPort = %s;\n' "${QUERY_PORT:-27016}" >> "$CONFIG"
	if [ -n "$MOTD" ]; then
		sed -i -E '/^motd(Interval)? *=|^motd\[\]/d' "$CONFIG"
		printf '\nmotd[] = { "%s" };\nmotdInterval = 300;\n' "$(echo "$MOTD" | sed 's/"/\\"/g; s/|/", "/g')" >> "$CONFIG"
	fi
fi

# RCON_PASSWORD (the VPS) enables BattlEye RCon on RCON_PORT, which is how the restart and
# wipe warnings reach players (tools/rcon.py). BattlEye renames the file to *_active_*.cfg
# when it reads it, so it is written fresh at every start.
if [ -n "$RCON_PASSWORD" ]; then
	rm -f battleye/beserver_x64_active_*.cfg
	printf 'RConPassword %s\nRConPort %s\n' "$RCON_PASSWORD" "${RCON_PORT:-2306}" > battleye/beserver_x64.cfg
fi

exec ./DayZServer "-config=$CONFIG" "-port=${GAME_PORT:-2302}" -profiles=profiles \
	"-mission=./mpmissions/$MISSION" "${MOD_ARGS[@]}" \
	-cpuCount=8 -limitFPS=200 -dologs -adminlog -freezecheck "$@"
