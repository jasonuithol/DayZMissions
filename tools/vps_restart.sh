#!/bin/bash
# Restart the public server (persistence kept) with the same in-game warnings as the
# nightly restart: 30, 10, 5 and 1 minutes, then stop and start. Needs tools/vps.conf.
#   tools/vps_restart.sh        asks, then starts the 30 minute countdown on the VPS
#   tools/vps_restart.sh -y     doesn't ask
#   tools/vps_restart.sh -n     right now, no warnings
#   tools/vps_restart.sh -c     cancel a countdown that is running
exec "$(dirname "$0")/vps_cycle.sh" restart "$@"
