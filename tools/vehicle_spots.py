#!/usr/bin/env python3
"""Turn the spawn rules in a mission's vehicles.json into spots.json, the list of places
(position + facing) with the vehicles to put there, from the vanilla mission's map data.

    tools/vehicle_spots.py bikespawns

vehicles.json:
  "rules": [{
     "name": "...",
     "buildings": ["Land_City_School", ...],   # spawn outside these building classes
                                               # (from mapgrouppos.xml), OR
     "events": ["VehicleCivilianSedan", ...],  # use the vanilla vehicle spawn points of
                                               # these events (from cfgeventspawns.xml)
     "types": "Motorbike_01_Blue|...",         # vehicle class, "A|B" = random pick in game
     "count": 1,                               # vehicles per spot
     "min_neighbours": 120,                    # only buildings in built-up areas: at least
     "neighbour_radius": 400,                  #   this many other buildings within this radius
     "min_spacing": 40,                        # one spot per cluster of places this close
     "max": 30                                 # at most this many spots, spread evenly
  }]

The mission caps the total whatever this produces; a few hundred vehicles is a real
load on the server, so raise both with care.
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


def vanilla_dir(terrain, game):
    server = 'DayZ Server Exp' if game == 'exp' else 'DayZServer'
    return os.path.join(STEAM, 'steamapps/common', server, 'mpmissions', 'dayzOffline.' + terrain)


def load_buildings(vanilla):
    pattern = re.compile(r'<group name="([^"]+)" pos="([^"]+)" rpy="[^"]+" a="([^"]+)"')
    buildings = []
    with open(os.path.join(vanilla, 'mapgrouppos.xml')) as f:
        for m in pattern.finditer(f.read()):
            x, y, z = (float(v) for v in m.group(2).split())
            buildings.append((m.group(1), x, y, z, float(m.group(3))))
    return buildings


def load_event_spawns(vanilla):
    """(event name, x, 0, z, heading) for every vanilla vehicle spawn point."""
    spawns = []
    with open(os.path.join(vanilla, 'cfgeventspawns.xml')) as f:
        for m in re.finditer(r'<event name="([^"]+)">(.*?)</event>', f.read(), re.S):
            for p in re.finditer(r'<pos x="([^"]+)" z="([^"]+)"(?: a="([^"]+)")?', m.group(2)):
                spawns.append((m.group(1), float(p.group(1)), 0.0, float(p.group(2)), float(p.group(3) or 0)))
    return spawns


def main():
    mdir = mission_dir(sys.argv[1])
    terrain = os.path.basename(mdir).split('.')[-1]
    game = 'exp'
    conf = os.path.join(mdir, 'mission.conf')
    if os.path.exists(conf):
        m = re.search(r'^GAME=(\w+)', open(conf).read(), re.M)
        if m:
            game = m.group(1)

    rules = json.load(open(os.path.join(mdir, 'vehicles.json')))['rules']
    vanilla = vanilla_dir(terrain, game)
    buildings = load_buildings(vanilla)
    events = load_event_spawns(vanilla)
    # only real buildings count as neighbours, not walls, wrecks or vegetation
    landmarks = [(x, z) for n, x, y, z, a in buildings if n.startswith('Land_') and 'wreck' not in n.lower()]

    def neighbours(x, z, radius):
        r2 = radius * radius
        return sum(1 for bx, bz in landmarks if (bx - x) ** 2 + (bz - z) ** 2 <= r2)

    spots = []
    for rule in rules:
        exact = 'events' in rule
        wanted = set(rule.get('events') or rule['buildings'])
        source = events if exact else buildings
        min_n = rule.get('min_neighbours', 0)
        radius = rule.get('neighbour_radius', 400)
        spacing = rule.get('min_spacing', 0)
        accepted = []
        candidates = []
        skipped = clustered = 0
        for name, x, y, z, a in source:
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
                          'pos': [round(x, 2), round(y, 2), round(z, 2)], 'heading': round(a, 2), 'exact': exact})
        print('%-40s %3d spots, %3d not built-up, %3d clustered, %3d vehicles' % (rule['name'], used, skipped, clustered, used * rule['count']))

    out = os.path.join(mdir, 'spots.json')
    with open(out, 'w') as f:
        json.dump({'spots': spots}, f, indent='\t')
        f.write('\n')
    print('wrote %s: %d spots, %d vehicles' % (os.path.relpath(out, PROJECT), len(spots), sum(s['count'] for s in spots)))


if __name__ == '__main__':
    main()
