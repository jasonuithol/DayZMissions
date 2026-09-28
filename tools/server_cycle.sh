#!/bin/bash
# Restart or wipe the public server with in-game warnings first. Runs ON the VPS as root:
# the dayz-restart / dayz-wipe timers start it, and tools/vps_restart.sh / vps_wipe.sh run
# it by hand. Players get a message at 30, 10, 5 and 1 minutes, then the server is
# stopped, its storage_* deleted for a wipe, and started again.
#   server_cycle.sh restart|wipe        warnings, then do it (takes 30 minutes)
#   server_cycle.sh restart|wipe now    no warnings
set -u
WHAT="${1:?restart|wipe}"
NOW="${2:-}"
REMOTE=/opt/dayz
[ -n "${RCON_PASSWORD:-}" ] || source $REMOTE/vps.env
: "${MISSION:?}"
DIR="$(cd "$(dirname "$0")" && pwd)"

say() {
	echo "say: $1"
	python3 "$DIR/rcon.py" 127.0.0.1 "${RCON_PORT:-2306}" "$RCON_PASSWORD" "say -1 $1" || echo "(rcon failed - server down?)"
}

if [ "$WHAT" = wipe ]; then
	EVENT="WEEKLY WIPE: the server wipes and restarts"
	AFTER="Everything resets - bases, stashes, characters, vehicles."
else
	EVENT="Server restart"
	AFTER="Your character, base and stash are kept."
fi

if [ "$NOW" != now ]; then
	say "$EVENT in 30 minutes. $AFTER"
	sleep 1200
	say "$EVENT in 10 minutes. $AFTER"
	sleep 300
	say "$EVENT in 5 minutes."
	sleep 240
	say "$EVENT in 1 MINUTE - log out somewhere safe."
	sleep 60
fi

systemctl stop dayz.service
if [ "$WHAT" = wipe ]; then
	rm -rf $REMOTE/steamapps/common/DayZServer/mpmissions/$MISSION.*/storage_* \
	       $REMOTE/steamapps/common/DayZServer/mpmissions/$MISSION/storage_*
	echo "wiped persistence"
fi
systemctl start dayz.service
echo "$WHAT done"
