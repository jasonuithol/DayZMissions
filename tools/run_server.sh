#!/bin/bash
# Deploy a mission and start the server on it. Vanilla - no mods.
#   tools/run_server.sh coastbikes
set -e
source "$(dirname "$0")/common.sh"
resolve_mission "$1"
shift

"$PROJECT_DIR/tools/deploy.sh" "$MISSION"

cd "$SERVER_DIR"
exec ./DayZServer -config=serverDZ.cfg -port=2302 -profiles=profiles \
	"-mission=./mpmissions/$MISSION" \
	-cpuCount=8 -limitFPS=200 -dologs -adminlog -freezecheck "$@"
