#!/bin/bash
# Wipe the public server without waiting for Friday. Same as the Friday timer: players are
# warned in game at 30, 10, 5 and 1 minutes, then the server stops, its storage_* is
# deleted and it starts fresh. Everyone on is kicked when it stops. Needs tools/vps.conf.
#   tools/vps_wipe.sh        asks, then starts the 30 minute countdown on the VPS
#   tools/vps_wipe.sh -y     doesn't ask
#   tools/vps_wipe.sh -n     right now, no warnings
#   tools/vps_wipe.sh -c     cancel a countdown that is running
exec "$(dirname "$0")/vps_cycle.sh" wipe "$@"
