#!/bin/bash
# Runs ON the VPS as root (piped in by vps_sync.sh). Creates the dayz user, writes the
# environment and the systemd unit, opens the firewall, (re)starts the server.
set -e
REMOTE=/opt/dayz
: "${MISSION:?}" "${SERVER_NAME:?}" "${GAME_PORT:=2302}" "${QUERY_PORT:=27016}"

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

# nightly restart at 05:00 server time: every start redeploys the mission, which wipes
# the persistence and puts every vehicle back where it belongs
cat > /etc/systemd/system/dayz-restart.service <<UNIT
[Unit]
Description=Nightly DayZ restart
[Service]
Type=oneshot
ExecStart=/bin/systemctl restart dayz.service
UNIT
cat > /etc/systemd/system/dayz-restart.timer <<UNIT
[Unit]
Description=Nightly DayZ restart
[Timer]
OnCalendar=*-*-* 05:00:00
Persistent=false
[Install]
WantedBy=timers.target
UNIT

if command -v ufw >/dev/null && ufw status | grep -q "Status: active"; then
	ufw allow "$GAME_PORT:$((GAME_PORT + 3))/udp" comment 'DayZ game + BattlEye' >/dev/null
	ufw allow "$QUERY_PORT/udp" comment 'DayZ Steam query' >/dev/null
	echo "ufw: opened UDP $GAME_PORT-$((GAME_PORT + 3)) and $QUERY_PORT"
fi

systemctl daemon-reload
systemctl enable --now dayz-restart.timer >/dev/null
systemctl enable dayz.service >/dev/null
systemctl restart dayz.service
sleep 3
systemctl --no-pager --lines=5 status dayz.service || true
echo "server logs: journalctl -u dayz -f ; script log: $REMOTE/steamapps/common/DayZServer/profiles/script_*.log"
