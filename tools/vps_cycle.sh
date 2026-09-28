#!/bin/bash
# Behind vps_restart.sh and vps_wipe.sh: runs the VPS's dayz-restart / dayz-wipe service
# (tools/server_cycle.sh there) in the background, or the immediate form over ssh.
source "$(dirname "$0")/common.sh"
CONF="$PROJECT_DIR/tools/vps.conf"
[ -f "$CONF" ] || { echo "missing $CONF" >&2; exit 1; }
source "$CONF"
WHAT="${1:?restart|wipe}"; shift
YES="" NOW="" CANCEL=""
for arg in "$@"; do
	case "$arg" in -y) YES=1 ;; -n) YES=1; NOW=1 ;; -c) CANCEL=1 ;; *) echo "usage: $0 restart|wipe [-y|-n|-c]" >&2; exit 1 ;; esac
done
UNIT="dayz-$WHAT.service"

if [ -n "$CANCEL" ]; then
	ssh "$VPS" "systemctl stop $UNIT; systemctl is-active $UNIT" || true
	echo "countdown cancelled (if one was running; the timers still fire as scheduled)"
	exit 0
fi

echo "players online now: $("$PROJECT_DIR/tools/vps_players.sh" 2>/dev/null || echo '?')"
if [ -z "$YES" ]; then
	read -r -p "$WHAT the public server (30 minute countdown with in-game warnings)? [y/N] " answer
	[ "$answer" = y ] || [ "$answer" = Y ] || { echo "left alone"; exit 1; }
fi
if [ -n "$NOW" ]; then
	ssh "$VPS" "/opt/dayz/DayZMissions/tools/server_cycle.sh $WHAT now"
else
	ssh "$VPS" "systemctl start --no-block $UNIT && echo 'countdown started; journalctl -u $UNIT -f shows it, $0 -c cancels it'"
fi
