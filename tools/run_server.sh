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
# SERVER_NAME / SERVER_PASSWORD / ADMIN_PASSWORD / QUERY_PORT in the environment (the
# VPS), get the mission its own copy of the server config; serverDZ.cfg is left alone.
CONFIG=serverDZ.cfg
if [ -n "$VERIFY_SIGNATURES$SERVER_NAME$SERVER_PASSWORD$ADMIN_PASSWORD$QUERY_PORT" ]; then
	CONFIG="serverDZ.${MISSION%%.*}.cfg"
	sed -E \
		-e "s/^verifySignatures *= *[0-9]+;/verifySignatures = ${VERIFY_SIGNATURES:-2};/" \
		${SERVER_NAME:+-e "s/^hostname *= *\"[^\"]*\";/hostname = \"$SERVER_NAME\";/"} \
		${SERVER_PASSWORD+-e "s/^password *= *\"[^\"]*\";/password = \"$SERVER_PASSWORD\";/"} \
		${ADMIN_PASSWORD:+-e "s/^passwordAdmin *= *\"[^\"]*\";/passwordAdmin = \"$ADMIN_PASSWORD\";/"} \
		serverDZ.cfg > "$CONFIG"
	grep -q "^steamQueryPort" "$CONFIG" || printf '\nsteamQueryPort = %s;\n' "${QUERY_PORT:-27016}" >> "$CONFIG"
fi

exec ./DayZServer "-config=$CONFIG" "-port=${GAME_PORT:-2302}" -profiles=profiles \
	"-mission=./mpmissions/$MISSION" "${MOD_ARGS[@]}" \
	-cpuCount=8 -limitFPS=200 -dologs -adminlog -freezecheck "$@"
