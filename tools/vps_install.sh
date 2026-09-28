#!/bin/bash
# Runs ON the VPS as root (piped in by vps_sync.sh). Creates the dayz user, writes the
# environment and the systemd unit, opens the firewall, (re)starts the server.
set -e
REMOTE=/opt/dayz
: "${MISSION:?}" "${SERVER_NAME:?}" "${GAME_PORT:=2302}" "${QUERY_PORT:=27016}" "${TIMEZONE:=Australia/Brisbane}"

# the restart and wipe timers, and the MOTD, are in this zone
[ "$(timedatectl show -p Timezone --value)" = "$TIMEZONE" ] || timedatectl set-timezone "$TIMEZONE"

# a swapfile as headroom: the modded server sits at ~5 GB and shares the box
if [ ! -f /swapfile ]; then
	fallocate -l 4G /swapfile && chmod 600 /swapfile && mkswap /swapfile >/dev/null && swapon /swapfile
	grep -q "^/swapfile" /etc/fstab || echo "/swapfile none swap sw 0 0" >> /etc/fstab
	echo "swap: 4 GB swapfile added"
fi

id -u dayz >/dev/null 2>&1 || useradd --system --home-dir $REMOTE --shell /usr/sbin/nologin dayz
chown -R dayz:dayz $REMOTE
chmod +x $REMOTE/DayZMissions/tools/*.sh $REMOTE/DayZMissions/tools/*.py $REMOTE/steamapps/common/DayZServer/DayZServer

cat > $REMOTE/vps.env <<ENV
STEAM=$REMOTE
NO_BUILD=1
SERVER_NAME="$SERVER_NAME"
SERVER_PASSWORD="$SERVER_PASSWORD"
ADMIN_PASSWORD="$ADMIN_PASSWORD"
GAME_PORT=$GAME_PORT
QUERY_PORT=$QUERY_PORT
MOTD="$MOTD"
RCON_PASSWORD="$RCON_PASSWORD"
RCON_PORT=${RCON_PORT:-2306}
MISSION=$MISSION
ENV
chmod 600 $REMOTE/vps.env

cat > /etc/systemd/system/dayz.service <<UNIT
[Unit]
Description=DayZ server ($MISSION)
After=network-online.target
Wants=network-online.target

[Service]
User=dayz
WorkingDirectory=$REMOTE/DayZMissions
EnvironmentFile=$REMOTE/vps.env
ExecStart=$REMOTE/DayZMissions/tools/run_server.sh $MISSION
Restart=always
RestartSec=20
LimitNOFILE=100000

[Install]
WantedBy=multi-user.target
UNIT

# nightly restart at 05:00 server time and weekly wipe Friday 17:00: the timers fire 30
# minutes early and tools/server_cycle.sh warns players in game (via RCon) at 30, 10, 5
# and 1 minutes before stopping the server. A restart keeps the persistence (the spawners
# only put back the vehicles that are missing); the wipe deletes it.
for what in restart wipe; do
	if [ $what = restart ]; then desc="Nightly DayZ restart (05:00)"; when="*-*-* 04:30:00"
	else desc="Weekly DayZ wipe (Friday 17:00)"; when="Fri *-*-* 16:30:00"; fi
	cat > /etc/systemd/system/dayz-$what.service <<UNIT
[Unit]
Description=$desc
[Service]
Type=oneshot
EnvironmentFile=$REMOTE/vps.env
ExecStart=$REMOTE/DayZMissions/tools/server_cycle.sh $what
UNIT
	cat > /etc/systemd/system/dayz-$what.timer <<UNIT
[Unit]
Description=$desc
[Timer]
OnCalendar=$when
Persistent=false
[Install]
WantedBy=timers.target
UNIT
done

# host firewall: ssh, the game ports and the query port are all the internet needs; RCon
# (RCON_PORT) is only ever used from the box itself and stays closed
if command -v ufw >/dev/null; then
	ufw allow OpenSSH >/dev/null
	ufw allow "$GAME_PORT:$((GAME_PORT + 3))/udp" comment 'DayZ game + BattlEye' >/dev/null
	ufw allow "$QUERY_PORT/udp" comment 'DayZ Steam query' >/dev/null
	ufw default deny incoming >/dev/null
	ufw --force enable >/dev/null
	echo "ufw: active - ssh, UDP $GAME_PORT-$((GAME_PORT + 3)) and $QUERY_PORT open, everything else closed"
fi

systemctl daemon-reload
systemctl enable --now dayz-restart.timer >/dev/null
systemctl enable --now dayz-wipe.timer >/dev/null
systemctl enable dayz.service >/dev/null
systemctl restart dayz.service
sleep 3
systemctl --no-pager --lines=5 status dayz.service || true
echo "timers ($TIMEZONE): $(systemctl list-timers dayz-restart.timer dayz-wipe.timer --no-pager --no-legend | awk '{print $1, $2, $3, $(NF-1)}' | tr '\n' ';')"
echo "server logs: journalctl -u dayz -f ; script log: $REMOTE/steamapps/common/DayZServer/profiles/script_*.log"
