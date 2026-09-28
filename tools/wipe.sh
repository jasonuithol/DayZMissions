#!/bin/bash
# Wipe a mission's persistence on the local server: deletes mpmissions/<mission>/storage_*
# (bases, stashes, characters, vehicles) so the next start is fresh. Stop the server first.
#   tools/wipe.sh roles        asks before deleting
#   tools/wipe.sh -y roles     doesn't
# For the public server use tools/vps_wipe.sh.
source "$(dirname "$0")/common.sh"
YES=""
[ "$1" = "-y" ] && { YES=1; shift; }
resolve_mission "$1"

TARGET="$SERVER_DIR/mpmissions/$MISSION"
shopt -s nullglob
STORES=("$TARGET"/storage_*)
if [ ${#STORES[@]} -eq 0 ]; then
	echo "$MISSION has no persistence to wipe ($TARGET)"
	exit 0
fi
if pgrep -f -- "-mission=./mpmissions/$MISSION" >/dev/null; then
	echo "the server is running on $MISSION - stop it first, or it will write the old state back" >&2
	exit 1
fi
for s in "${STORES[@]}"; do du -sh "$s"; done
if [ -z "$YES" ]; then
	read -r -p "delete? [y/N] " answer
	[ "$answer" = y ] || [ "$answer" = Y ] || { echo "left alone"; exit 1; }
fi
rm -rf "${STORES[@]}"
echo "wiped $MISSION - the next start is a fresh world"
