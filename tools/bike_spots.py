#!/usr/bin/env python3
"""Turn the spawn rules in a mission's bikes.json into spots.json, a list of buildings
(position + facing) with the bikes to put outside each one, using the vanilla
mission's mapgrouppos.xml.

    tools/bike_spots.py bikespawns

bikes.json:
  "rules": [{
     "name": "...",
     "buildings": ["Land_City_School", ...],   # building classes to spawn outside
     "types": "Motorbike_01_Blue|...",         # bike class, "A|B" = random pick in game
     "count": 3,                               # bikes per building
     "min_neighbours": 120,                    # only buildings in built-up areas: at least
     "neighbour_radius": 400,                  #   this many other buildings within this radius
     "min_spacing": 40,                        # one spot per cluster of buildings this close
     "max": 30                                 # at most this many buildings, spread evenly
  }]

The mission stops at its MAX_BIKES (160) whatever this produces; a few hundred vehicles
is a real load on the server, so raise both with care.
"""
import json, math, os, re, sys

sys.path.insert(0, os.path.dirname(__file__))
PROJECT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STEAM = os.environ.get('STEAM', os.path.expanduser('~/.steam/debian-installation'))


def mission_dir(name):
    d = os.path.join(PROJECT, 'missions', name)
    if not os.path.isdir(d):
        matches = [m for m in os.listdir(os.path.join(PROJECT, 'missions')) if m.startswith(name + '.')]
        if matches:
            d = os.path.join(PROJECT, 'missions', matches[0])
    return d


def load_buildings(terrain, game):
    server = 'DayZ Server Exp' if game == 'exp' else 'DayZServer'
    path = os.path.join(STEAM, 'steamapps/common', server, 'mpmissions', 'dayzOffline.' + terrain, 'mapgrouppos.xml')
    pattern = re.compile(r'<group name="([^"]+)" pos="([^"]+)" rpy="[^"]+" a="([^"]+)"')
    buildings = []
    with open(path) as f:
        for m in pattern.finditer(f.read()):
            x, y, z = (float(v) for v in m.group(2).split())
            buildings.append((m.group(1), x, y, z, float(m.group(3))))
    return buildings


def main():
    mdir = mission_dir(sys.argv[1])
    terrain = os.path.basename(mdir).split('.')[-1]
    game = 'exp'
    conf = os.path.join(mdir, 'mission.conf')
    if os.path.exists(conf):
        m = re.search(r'^GAME=(\w+)', open(conf).read(), re.M)
        if m:
            game = m.group(1)

    rules = json.load(open(os.path.join(mdir, 'bikes.json')))['rules']
    buildings = load_buildings(terrain, game)
    # only real buildings count as neighbours, not walls, wrecks or vegetation
    landmarks = [(x, z) for n, x, y, z, a in buildings if n.startswith('Land_') and 'wreck' not in n.lower()]

    def neighbours(x, z, radius):
        r2 = radius * radius
        return sum(1 for bx, bz in landmarks if (bx - x) ** 2 + (bz - z) ** 2 <= r2)

    spots = []
    for rule in rules:
        wanted = set(rule['buildings'])
        min_n = rule.get('min_neighbours', 0)
        radius = rule.get('neighbour_radius', 400)
        spacing = rule.get('min_spacing', 0)
        accepted = []
        candidates = []
        skipped = clustered = 0
        for name, x, y, z, a in buildings:
            if name not in wanted:
                continue
            if min_n and neighbours(x, z, radius) < min_n:
                skipped += 1
                continue
            if spacing and any((ax - x) ** 2 + (az - z) ** 2 < spacing * spacing for ax, az in accepted):
                clustered += 1
                continue
            accepted.append((x, z))
            candidates.append((name, x, y, z, a))
        # every k-th candidate: mapgrouppos is sorted by x, so this spreads them across the map
        limit = rule.get('max', len(candidates))
        if limit < len(candidates):
            step = len(candidates) / float(limit)
            candidates = [candidates[int(i * step)] for i in range(limit)]
        used = len(candidates)
        for name, x, y, z, a in candidates:
            spots.append({'building': name, 'types': rule['types'], 'count': rule['count'],
                          'pos': [round(x, 2), round(y, 2), round(z, 2)], 'heading': round(a, 2)})
        print('%-40s %3d buildings, %3d not built-up, %3d clustered, %3d bikes' % (rule['name'], used, skipped, clustered, used * rule['count']))

    out = os.path.join(mdir, 'spots.json')
    with open(out, 'w') as f:
        json.dump({'spots': spots}, f, indent='\t')
        f.write('\n')
    print('wrote %s: %d spots, %d bikes' % (os.path.relpath(out, PROJECT), len(spots), sum(s['count'] for s in spots)))


if __name__ == '__main__':
    main()
