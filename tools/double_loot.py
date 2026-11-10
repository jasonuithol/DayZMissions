#!/usr/bin/env python3
"""Write missions/<mission>/db/types.xml: the terrain's vanilla loot table with nominal and
min doubled for everything except vehicles (anything a mission event spawns as a vehicle).

    tools/double_loot.py <mission> <terrain>     e.g. roles.nasdara nasdara

Regenerate after a game update changes the vanilla types.xml.
"""
import os, re, sys
import xml.etree.ElementTree as ET

mission, terrain = sys.argv[1:3]
project = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
steam = os.environ.get('STEAM', os.path.expanduser('~/.steam/debian-installation'))
source = f'{steam}/steamapps/common/DayZServer/mpmissions/dayzOffline.{terrain}/db/types.xml'
db = f'{project}/missions/{mission}/db'

events = open(f'{db}/events.xml', encoding='utf-8', errors='replace').read()
vehicles = set()
for name, body in re.findall(r'<event name="(Vehicle[^"]*)">(.*?)</event>', events, re.S):
    vehicles.update(re.findall(r'<child [^>]*type="([^"]+)"', body))

doubled = skipped = 0
def double(match):
    global doubled, skipped
    if match.group(1) in vehicles:
        skipped += 1
        return match.group(0)
    body = re.sub(r'<(nominal|min)>(\d+)</\1>', lambda m: f'<{m[1]}>{int(m[2]) * 2}</{m[1]}>', match.group(2))
    doubled += body != match.group(2)
    return f'<type name="{match.group(1)}">{body}</type>'

text = open(source, encoding='utf-8', errors='replace').read()
out = re.sub(r'<type name="([^"]+)">(.*?)</type>', double, text, flags=re.S)
ET.fromstring(out.encode('utf-8'))
with open(f'{db}/types.xml', 'w', encoding='utf-8') as f:
    f.write(out)
print(f'{doubled} types doubled, {skipped} vehicles left alone, {len(vehicles)} vehicle classes in events')
