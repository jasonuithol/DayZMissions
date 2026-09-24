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
  `mods/VehicleShootingAnywhere` (built into `build/` by `tools/build_mod.py`, listed as `local`
  in `mods.txt`) opens it up to every passenger seat and every firearm. It is unsigned, so the
  mission sets `VERIFY_SIGNATURES=0` and other players need a copy of `build/@VehicleShootingAnywhere`.
- `mods/HeliHuntCompat` retunes the Honda's steering and tyres (more lock, faster response, open
  centre differential, more grip) - the values are in `mods/HeliHuntCompat/config.cpp`.
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
vehicle spawn points (spread evenly over the ~380 the map defines). Plus Expansion vehicles where
they belong: buses at bus stations, schools, the ferris wheels, town squares (three or more kiosks
together in a big town) and the big summer camps; tractors at the big metal sheds, long barns and
cowsheds; Vodniks at military sites with water within 150 m (checked in game); and an LHD
assault ship anchored 300-650 m off every port (the pier-crane clusters: Chernogorsk, Elektro,
Berezino, Svetlojarsk). And a DayZ Expansion helicopter
on each of the five helipads on the map - Huey, Little Bird, Merlin and Gyrocopter, each once,
plus a random fifth, shuffled - which is why this mission loads CF, Dabs Framework and the
three Expansion mods (`mods.txt`). Passengers can shoot from every vehicle (Vehicle Shooting +
Survivor Animations + our unsigned `VehicleShootingAnywhere`, as in helihunt, so
`VERIFY_SIGNATURES=0`). Players spawn beside the bus at the Chernogorsk bus station.

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
per spot, how built-up the area must be, spacing, a cap per rule, `cluster` to use the centre of
groups of buildings, `near_water` and `offshore` for the game to check); `tools/vehicle_spots.py
bikespawns` turns them into `spots.json` using the vanilla `mapgrouppos.xml` /
`cfgeventspawns.xml`, and `lib/VehicleSpots.c` does the placement in game (waits 20 s after
start, spawns in batches, respawns anything that came up empty or fell through a floor).
Re-run the tool after editing the rules. `tools/test.sh bikespawns` reports upright / fuelled /
fallen-through counts; `TEST_ARGS=-bikelimit=N` caps a test run.

### roadprobe.chernarusplus / objprobe.chernarusplus
Dev tools. `roadprobe` scans north-south columns and logs the surface types (1.30).
`objprobe` (stable) lists map objects around a point (`TEST_ARGS=-probe=x,z,radius,filter`;
within 20 m it also shows nameless terrain objects - decals, rocks, sandbags - with their
model and size), finds every object on the map by model name (`-findmodel=decal_heli`),
raycasts a point (`-ray=x,z`) or prints an ASCII map of surface types (`-surf=x,z,radius,step`).

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
- Expansion's LHD logs a "Virtual Machine Exception" on 1.29 from its own `DeferredInit`
  (`CallLater(Update)` with the wrong signature) - the mod's bug, the ship still works.
- There is no ski resort on Chernarus (no ski-lift or chalet classes in the map data).
- The server process names its main thread `enfMain`, so `pgrep DayZServer` finds nothing;
  match on the command line (`ps -eo rss,args | grep '[D]ayZServer'`) to watch its memory.
- `foreach` over an array that comes from a function call (`foreach (X x : Foo())`) or from a
  member of a loop variable is unreliable: it raised "Virtual Machine Exception" and made a
  weighted random pick return the first element every time. Index loops are safe.

## Going public: the VPS plan (not started)

The server has to live somewhere with open ports to appear in the in-game browser or
DZSA (both list the address Steam's master server sees, so tunnels and VPNs don't help;
there is no router access here). The VPS from the Translink project
(`root@<vps host>`, the VPS provider) is the target. Tooling is written and
dry-tested, nothing has been run against the box yet.

1. `ssh-copy-id root@<vps host>` from this machine - the key here is not
   authorised on the VPS yet (Translink deploys used a password).
2. Copy `tools/vps.conf.example` to `tools/vps.conf` (git-ignored): server name, join
   password (empty = public), admin password, ports.
3. Check the box. Done 2026-09-24: Ubuntu 26.04, 2 vCPU, **3.8 GB RAM (2.3 GB free, no
   swap)**, **20 GB disk with 3.2 GB free**, ufw inactive, ssh key now authorised. That is
   too small. Measured 2026-09-24: the `roles` server (Expansion, 215 vehicles, 5
   helicopters) sits at **4.9-5.0 GB RSS** as soon as the world is loaded, before any
   player joins. Removing the Translink and InventoryQuest apps from the box frees ~1 GB
   RAM and ~11 GB disk, which is not enough. **Needs 8 GB RAM / 40 GB disk**: resize the
   the VPS provider VPS or take a second one, then continue at step 4. The panel firewall must
   also allow UDP 2302-2305 and 27016.
4. `tools/vps_sync.sh` - rsyncs the stable server (3.8 GB), the mission's Workshop mods
   (1.4 GB for roles) and this project to `/opt/dayz`, laid out like a Steam library so
   the tools run unchanged with `STEAM=/opt/dayz`; then runs `tools/vps_install.sh` on
   the box: `dayz` user, `dayz.service` (auto-restart), nightly 05:00 restart timer
   (every start redeploys = wipes persistence and resets the vehicles), ufw rules. No
   Steam login on the VPS - SteamCMD can't fetch DayZ's server or Workshop mods
   anonymously, so everything is shipped from here; re-run the sync to update.
5. `run_server.sh` reads `SERVER_NAME`, `SERVER_PASSWORD`, `ADMIN_PASSWORD`, `QUERY_PORT`
   from the environment (the service's `vps.env`) into the generated per-mission config.
6. Decide about `VehicleShootingAnywhere`: it is unsigned, so the server runs with
   `verifySignatures = 0` and launchers can't fetch it. Friends-only: send them the
   `build/@VehicleShootingAnywhere` folder. Public: sign it and publish it to the
   Workshop (DayZ Tools is installed; Windows tools, Proton) or drop it.
7. Then: it should appear in the Community tab and DZSA within minutes; the launcher
   installs the Workshop mods for players.

## Open items

- **Helipads** - Chernarus has exactly five, all `decal_heli_army.p3d` (a nameless terrain
  object; `objprobe -findmodel=decal_heli` lists them): Chernogorsk camp 7236/3063, Balota
  5030/2356 and 5055/2333, Vybor military base 4156/11028 and 4169/10991. All have a helicopter
  in `roles`. Expansion's planes (An-2, C-130J) are `scope 0` like its bikes - not spawnable.
- **helihunt bike handling** - the CRF450R still slides; the current tuning (grip 1.9, 38 deg lock,
  faster steering, open centre diff) is in `mods/HeliHuntCompat/config.cpp` and hasn't been judged
  in game yet. Next experiment: put the centre differential back to `DIFFERENTIAL_LOCKED`.
- **Expansion on 1.30** - recheck the DayZ-Expansion-Vehicles(-Experimental) change notes; once it
  compiles on 1.30, helicopters and the vanilla bikes can share one mission.
- **Friend pack** - other players need the Workshop mods plus a copy of `build/@HeliHuntCompat`;
  a zip + Windows launch shortcut would make that easier.
- **Multiplayer spawns** - both missions spawn everyone on the same spot; spread them out, or give
  hunters and riders separate spawns.
