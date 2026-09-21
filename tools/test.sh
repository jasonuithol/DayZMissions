#!/bin/bash
# Headless smoke test: deploy a mission, boot the server with -missiontest and a
# throwaway profile dir, and show the mission's log lines plus any script errors.
# Missions that support it log their state and shut the server down by themselves.
source "$(dirname "$0")/common.sh"
resolve_mission "$1"

"$PROJECT_DIR/tools/deploy.sh" "$MISSION" || exit 1

PROFILES="$(mktemp -d)"
cd "$SERVER_DIR" || exit 1
./DayZServer -config=serverDZ.cfg -port=2302 \
	"-profiles=$PROFILES" "-mission=./mpmissions/$MISSION" -dologs -missiontest=1 \
	> "$PROFILES/stdout.txt" 2>&1 &
SERVER_PID=$!

# a server with a broken init.c sits there forever, so stop it as soon as that shows up
for ((t = 0; t < ${TEST_TIMEOUT:-180}; t++)); do
	kill -0 $SERVER_PID 2>/dev/null || break
	grep -q "Can't compile" "$PROFILES"/script_*.log 2>/dev/null && break
	sleep 1
done
kill -9 $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "logs: $PROFILES"
grep -h -E "SCRIPT +: \[|SCRIPT +\(E\)|Can't compile" "$PROFILES"/script_*.log | grep -v "Leaked\|Total Leaks"
