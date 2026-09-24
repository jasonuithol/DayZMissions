#!/bin/bash
# Ship the DayZ server, the mission's Workshop mods and this project to the VPS and
# (re)install the systemd service there. Re-run after any change; rsync only sends
# what differs. Needs tools/vps.conf (see vps.conf.example) and key-based ssh.
#
# VPS layout mirrors a Steam library so the tools work unchanged with STEAM=/opt/dayz:
#   /opt/dayz/steamapps/common/DayZServer            the stable server (from this machine)
#   /opt/dayz/steamapps/workshop/content/221100/<id>  the mods the mission lists
#   /opt/dayz/DayZMissions                           this project
set -e
source "$(dirname "$0")/common.sh"
CONF="$PROJECT_DIR/tools/vps.conf"
[ -f "$CONF" ] || { echo "missing $CONF - copy tools/vps.conf.example and fill it in" >&2; exit 1; }
source "$CONF"
resolve_mission "$MISSION"
[ "$GAME" = stable ] || { echo "the VPS runs the stable server; $MISSION is GAME=$GAME" >&2; exit 1; }

REMOTE=/opt/dayz
RSYNC="rsync -az --info=progress2 --delete"

echo "==> server ($SERVER_DIR)"
ssh "$VPS" "mkdir -p $REMOTE/steamapps/common $REMOTE/steamapps/workshop/content/221100"
$RSYNC --exclude 'profiles/' --exclude 'mpmissions/*/storage_*' --exclude 'mpmissions/[!d]*' \
	"$SERVER_DIR/" "$VPS:$REMOTE/steamapps/common/DayZServer/"

echo "==> mods for $MISSION"
if [ -f "$PROJECT_DIR/missions/$MISSION/mods.txt" ]; then
	while read -r id name; do
		case "$id" in ""|\#*|local) continue ;; esac
		echo "    $name ($id)"
		$RSYNC "$WORKSHOP_DIR/$id/" "$VPS:$REMOTE/steamapps/workshop/content/221100/$id/"
	done < "$PROJECT_DIR/missions/$MISSION/mods.txt"
fi

echo "==> project"
$RSYNC --exclude 'build/' --exclude '.git/' "$PROJECT_DIR/" "$VPS:$REMOTE/DayZMissions/"

echo "==> installing the service"
ssh "$VPS" MISSION="$MISSION" SERVER_NAME="$SERVER_NAME" SERVER_PASSWORD="$SERVER_PASSWORD" \
	ADMIN_PASSWORD="$ADMIN_PASSWORD" GAME_PORT="$GAME_PORT" QUERY_PORT="$QUERY_PORT" \
	'bash -s' < "$PROJECT_DIR/tools/vps_install.sh"
