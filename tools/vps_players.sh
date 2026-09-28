#!/bin/bash
# How many players are on the public server (Steam A2S query, from tools/vps.conf).
#   tools/vps_players.sh       prints the count
#   tools/vps_players.sh -v    also the names, when the server gives them
source "$(dirname "$0")/common.sh"
source "$PROJECT_DIR/tools/vps.conf" || exit 1
python3 - "${VPS#*@}" "${QUERY_PORT:-27016}" "$1" <<'PY'
import socket, sys
host, port, verbose = sys.argv[1], int(sys.argv[2]), sys.argv[3] == '-v'
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.settimeout(4); a = (host, port)
q = b'\xff\xff\xff\xffTSource Engine Query\x00'
s.sendto(q, a); d, _ = s.recvfrom(4096)
if d[4:5] == b'A': s.sendto(q + d[5:9], a); d, _ = s.recvfrom(4096)
p = d[6:]; i = 0
for _ in range(4): i = p.index(b'\0', i) + 1
if verbose:
    print('%d / %d' % (p[i + 2], p[i + 3]))
    s.sendto(b'\xff\xff\xff\xffU\xff\xff\xff\xff', a); d, _ = s.recvfrom(4096)
    if d[4:5] == b'A': s.sendto(b'\xff\xff\xff\xffU' + d[5:9], a); d, _ = s.recvfrom(4096)
    if d[4:5] == b'D':
        pos = 6
        for _ in range(d[5]):
            end = d.index(b'\0', pos + 1); print('  ' + (d[pos + 1:end].decode(errors='replace') or '(no name)')); pos = end + 9
else:
    print(p[i + 2])
PY
