#!/usr/bin/env python3
"""BattlEye RCon client: rcon.py <host> <port> <password> <command...>
Sends one command and prints the reply. Used by server_cycle.sh for in-game warnings
("say -1 <text>" is a message to everyone); "players" lists who is on."""
import socket, struct, sys, zlib

def packet(payload):
    return b'BE' + struct.pack('<I', zlib.crc32(payload) & 0xffffffff) + payload

def main():
    host, port, password = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    command = ' '.join(sys.argv[4:])
    # not connect(): BattlEye answers from another address, which a connected socket drops
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(5)
    server = (host, port)
    s.sendto(packet(b'\xff\x00' + password.encode()), server)
    reply, _ = s.recvfrom(4096)
    if reply[6:9] != b'\xff\x00\x01':
        sys.exit('rcon: login refused')
    s.sendto(packet(b'\xff\x01\x00' + command.encode()), server)
    parts, expected = {}, 1
    while len(parts) < expected:
        try:
            reply, _ = s.recvfrom(65536)
        except socket.timeout:
            break
        body = reply[6:]
        if body[1:2] == b'\x02':               # a server message: acknowledge and ignore
            s.sendto(packet(b'\xff\x02' + body[2:3]), server)
            continue
        if body[1:2] != b'\x01':
            continue
        data = body[3:]
        if data[:1] == b'\x00' and len(data) >= 3:   # multi-part reply
            expected, index, data = data[1], data[2], data[3:]
        else:
            index = 0
        parts[index] = data
    text = b''.join(parts[i] for i in sorted(parts)).decode('utf-8', 'replace').strip()
    if text:
        print(text)

if __name__ == '__main__':
    main()
