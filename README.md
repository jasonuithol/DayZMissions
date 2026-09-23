# DayZ Missions

Scripted missions for DayZ dedicated servers on Linux, stable and Experimental. Missions are
server-side `init.c` scripting; some are pure vanilla, some load Workshop mods.

## Usage

```bash
tools/run_server.sh coastbikes            # deploy + start the right server for a mission
tools/run_client.sh coastbikes [ip]       # start the matching client (no launcher) and connect; default 127.0.0.1
tools/test.sh coastbikes                  # headless smoke test, prints the mission's log lines
```

Each mission picks its game with `GAME=exp|stable` in `mission.conf` (default `exp`). Servers and
clients are expected in the Steam library under `~/.steam/debian-installation` (override with `STEAM`).

## Layout

- `missions/<name>.<terrain>/` - one folder per mission. Only the files that differ from
  vanilla go here (normally just `init.c`).
- `mods/` - our own small mods, packed into `build/` on demand.
- `lib/` - script shared between missions. Pull it into a mission's `init.c` with
  `#include "lib/Foo.c"`; `deploy.sh` inlines the file, because the engine can't resolve
  includes relative to the mission folder.
- `tools/` - deploy / run / test scripts.

`deploy.sh` builds `mpmissions/<name>.<terrain>` in the server folder: vanilla
`dayzOffline.<terrain>` files are symlinked and the mission's files copied over them.
There is no `storage_1`, so **every deploy is a wipe** - script-spawned vehicles would
otherwise be duplicated on each restart, and players get a fresh spawn at the mission start.

## Mods

A mission opts into mods with a `mods.txt` next to its `init.c`: one `<workshop id> <@Name>`
per line, in load order. Subscribe to them in Steam; the scripts symlink them into the server and
client folders, install the server keys and pass `-mod=`. `NOMODS=1` ignores the file.

## Missions

### coastbikes.chernarusplus
All seven motorbikes (`Motorbike_01` blue/red/yellow, `Motorbike_02` blue/green/red/yellow)
lined up 2 m apart on the coast highway half way between Chernogorsk and Elektrozavodsk,
with wheels, headlight, spark plug, colour matched shields and a full tank. In front of each
bike: enduro helmet (with visor and mouthguard), leather jacket, jeans, hiking boots, aviators
and leather gloves. Players spawn just behind the line-up, at 09:00.

The road isn't hard-coded: `RoadFinder` searches outwards from an anchor point for the nearest
asphalt, then works out the centre line and heading, so moving `ANCHOR` moves the whole scene.

### helihunt.chernarusplus (stable 1.29, modded)
Hunters and hunted. Six MBM Honda CRF450R dirt bikes with riding gear lined up on the coast
highway, and 30 m either side of them a UH-1H Huey (6 seats, rear doors left off for door
gunners) and an MH-6 Little Bird (4 seats) with every fluid topped up (Expansion helicopters
need hydraulic fluid as well as fuel). Beside each helicopter: military clothing, a loaded M4A1
and SVD, spare magazines, ammo and kit. Passengers can shoot from the helicopters.

Subscribe in Steam to: CF, Dabs Framework, DayZ-Expansion-Licensed, -Core and -Vehicles,
MBM_HondaCRF450R, Survivor Animations and Vehicle Shooting (ids in `mods.txt`).

- It runs on stable because, as of 2026-09-21, Expansion and Dabs don't compile on 1.30
  Experimental (1.30 reworked the vehicle script API). Expansion's own motorbikes are disabled
  in the mod (`scope 0`), hence the Honda.
- Vehicle Shooting only allows pistols, in vehicles with vanilla seat animations. Our own
  `mods/HeliHuntCompat` (built into `build/` by `tools/build_mod.py`, listed as `local` in
  `mods.txt`) opens it up to every passenger seat and every firearm. It is unsigned, so the
  mission sets `VERIFY_SIGNATURES=0` and other players need a copy of `build/@HeliHuntCompat`.
- HeliHuntCompat also retunes the Honda's steering (more lock, faster response, open centre
  differential) - the values are in `mods/HeliHuntCompat/config.cpp`.
- The bikes are the handlebar variants, which need Survivor Animations for the riding pose;
  the `MBM_HondaCRF450_W_<Colour>` ones have a steering wheel and don't.

### roles.chernarusplus (stable 1.29)
Vanilla Chernarus, but every new character spawns as one of 22 roles - police officer, doctor,
nurse, paramedic, soldier, military police, sniper, tank crew, pilot, technician, tradesman,
hunter, hiker, motorcyclist, firefighter, lumberjack, prisoner, journalist, athlete, executive,
NBC specialist, medieval re-enactor - in the matching outfit with a few things that fit the job
(the doctor's first-aid kit is stocked, the lumberjack holds an axe, the police officer has
handcuffs and a radio...). No guns. Vanilla freshie basics (bandage, chemlight, fruit) on top.
And 150 ready-to-drive cars and trucks, every part fitted and every fluid full, at the vanilla
vehicle spawn points (spread evenly over the ~380 the map defines). And a DayZ Expansion MH-6
Little Bird, fuelled and ready, on the helipad of the military camp east of Chernogorsk (the
packed-dirt square between the two fortified nests, ~7237/3065) - which is why this mission
loads CF, Dabs Framework and the three Expansion mods (`mods.txt`).

The roles are data: `roles.json` next to `init.c`, one entry per role with `clothing` and
`items` (each item can have `attachments`, `cargo` and `hands: true`). `"A|B|C"` picks one
at random. `lib/Roles.c` loads and applies it; `tools/test.sh roles` checks every class name
exists and every container actually takes what it is given. The cars come from `vehicles.json`
via `tools/vehicle_spots.py roles` (see bikespawns below).

### roles-exp.chernarusplus (1.30 Experimental)
The same roles on 1.30, without the cars. 1.30 has motorbikes but, as of 2026-09-23, also
zombie attacks that land from where the attack started and doors whose interaction point is
off - so the stable one above is the mission to play until that is fixed. `roles.json` differs
slightly (1.30's first-aid kit holds one more item, and has the waterskin).

### bikespawns.chernarusplus (1.30 Experimental)
Vanilla Chernarus with about 160 ready-to-ride motorbikes, one per spot: mopeds (`Motorbike_01`)
outside schools, police stations, shops, hospitals, petrol stations and some apartment blocks in
the built-up towns, dirt bikes (`Motorbike_02`) at rail warehouses, big garages, trail-head
shelters and one per summer camp. Each bike gets a
clear patch of ground near its building - the kerb of the nearest road if there is one, otherwise
open ground, and failing that a roof, platform or floor (never clipped into walls). A bike that
drops through an interior floor is respawned on open ground, and if that keeps swallowing it, on
the building's roof - the occasional rooftop bike is a feature.

`vehicles.json` holds the rules (building classes or vanilla spawn events, vehicle types, count
per spot, how built-up the area must be, spacing, a cap per rule); `tools/vehicle_spots.py
bikespawns` turns them into `spots.json` using the vanilla `mapgrouppos.xml` /
`cfgeventspawns.xml`, and `lib/VehicleSpots.c` does the placement in game (waits 20 s after
start, spawns in batches, respawns anything that came up empty or fell through a floor).
Re-run the tool after editing the rules. `tools/test.sh bikespawns` reports upright / fuelled /
fallen-through counts; `TEST_ARGS=-bikelimit=N` caps a test run.

### roadprobe.chernarusplus / objprobe.chernarusplus
Dev tools. `roadprobe` scans north-south columns and logs the surface types (1.30).
`objprobe` (stable) lists map objects around a point (`TEST_ARGS=-probe=x,z,radius,filter`)
or prints an ASCII map of surface types (`TEST_ARGS=-surf=x,z,radius,step`) - that is how
the Chernogorsk helipad was found: it is not an object, just a square of `dirt_ext`.

## Writing missions - things learned

- Roads report surface type `asphalt_ext` via `SurfaceGetType3D`. Asphalt where
  `SurfaceRoadY - SurfaceY > 0` is a roof/platform, not a road.
- Enforce Script has no `%` for this usage and dislikes array literals as call arguments
  or containing non-constants - build arrays with `Insert`.
- A server with an `init.c` compile error hangs rather than exits; `test.sh` detects this.
- `-missiontest` on the command line is the convention for "log state after 20 s and quit".
- `GetMissionFolderPath()` is empty on a dedicated server; the path handed to
  `CreateCustomMission` (`./mpmissions/<mission>/mission.c`) is the way to find mission files.
- `JsonFileLoader<T>` maps JSON keys to class members; `set` is a reserved word. On 1.29 it
  won't take mission-script classes, and `JsonSerializer.ReadFromString` fills nothing if the
  object arrives through a `Class`-typed parameter - deserialise with a typed variable
  (`lib/JsonFile.c`). 1.29 also needs `$CurrentDir:` on mission file paths.
- Vehicles created in the first ~10 s after server start end up with no fuel and can't be
  filled later, ever. `Fill()` right after `CreateObjectEx` also reads back as 0 for a moment
  even when it worked. Spawn vehicles from a `CallLater`, and check fuel a few seconds on.
- `IsBoxCollidingGeometry` + `SurfaceY`/`SurfaceRoadY` (roads and roofs) is enough to find
  clear ground; see `lib/Placement.c`.
- `foreach` over an array that comes from a function call (`foreach (X x : Foo())`) or from a
  member of a loop variable is unreliable: it raised "Virtual Machine Exception" and made a
  weighted random pick return the first element every time. Index loops are safe.

## Open items

- **helihunt bike handling** - the CRF450R still slides; the current tuning (grip 1.9, 38 deg lock,
  faster steering, open centre diff) is in `mods/HeliHuntCompat/config.cpp` and hasn't been judged
  in game yet. Next experiment: put the centre differential back to `DIFFERENTIAL_LOCKED`.
- **Expansion on 1.30** - recheck the DayZ-Expansion-Vehicles(-Experimental) change notes; once it
  compiles on 1.30, helicopters and the vanilla bikes can share one mission.
- **Friend pack** - other players need the Workshop mods plus a copy of `build/@HeliHuntCompat`;
  a zip + Windows launch shortcut would make that easier.
- **Multiplayer spawns** - both missions spawn everyone on the same spot; spread them out, or give
  hunters and riders separate spawns.
