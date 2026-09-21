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

# VERIFY_SIGNATURES in mission.conf (needed for our own unsigned mods) gets the mission
# its own copy of the server config; serverDZ.cfg itself is left alone.
CONFIG=serverDZ.cfg
if [ -n "$VERIFY_SIGNATURES" ]; then
	CONFIG="serverDZ.${MISSION%%.*}.cfg"
	sed -E "s/^verifySignatures *= *[0-9]+;/verifySignatures = $VERIFY_SIGNATURES;/" serverDZ.cfg > "$CONFIG"
fi

exec ./DayZServer "-config=$CONFIG" -port=2302 -profiles=profiles \
	"-mission=./mpmissions/$MISSION" "${MOD_ARGS[@]}" \
	-cpuCount=8 -limitFPS=200 -dologs -adminlog -freezecheck "$@"
