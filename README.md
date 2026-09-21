# DayZ Missions

Scripted missions for the DayZ Experimental server. Everything is vanilla server-side
`init.c` scripting - no mods on the server or the client.

## Usage

```bash
tools/run_server.sh coastbikes   # deploy + start the server on a mission (no mods)
tools/run_client.sh [ip[:port:queryport]]   # start the client (no mods, no launcher) and connect; default 127.0.0.1
tools/test.sh coastbikes         # headless smoke test, prints the mission's log lines
```

The server lives in `~/.steam/steam/steamapps/common/DayZ Server Exp` (override with `SERVER_DIR`).

## Layout

- `missions/<name>.<terrain>/` - one folder per mission. Only the files that differ from
  vanilla go here (normally just `init.c`).
- `lib/` - script shared between missions. Pull it into a mission's `init.c` with
  `#include "lib/Foo.c"`; `deploy.sh` inlines the file, because the engine can't resolve
  includes relative to the mission folder.
- `tools/` - deploy / run / test scripts.

`deploy.sh` builds `mpmissions/<name>.<terrain>` in the server folder: vanilla
`dayzOffline.<terrain>` files are symlinked and the mission's files copied over them.
There is no `storage_1`, so **every deploy is a wipe** - script-spawned vehicles would
otherwise be duplicated on each restart, and players get a fresh spawn at the mission start.

## Missions

### coastbikes.chernarusplus
All seven motorbikes (`Motorbike_01` blue/red/yellow, `Motorbike_02` blue/green/red/yellow)
lined up 2 m apart on the coast highway half way between Chernogorsk and Elektrozavodsk,
with wheels, headlight, spark plug, colour matched shields and a full tank. In front of each
bike: enduro helmet (with visor and mouthguard), leather jacket, jeans, hiking boots, aviators
and leather gloves. Players spawn just behind the line-up, at 09:00.

The road isn't hard-coded: `RoadFinder` searches outwards from an anchor point for the nearest
asphalt, then works out the centre line and heading, so moving `ANCHOR` moves the whole scene.

### roadprobe.chernarusplus
Dev tool. Scans north-south columns, logs the surface types to the script log and exits.
Handy for finding roads / anchor points for new missions (`tools/test.sh roadprobe`).

## Writing missions - things learned

- Roads report surface type `asphalt_ext` via `SurfaceGetType3D`. Asphalt where
  `SurfaceRoadY - SurfaceY > 0` is a roof/platform, not a road.
- Enforce Script has no `%` for this usage and dislikes array literals as call arguments
  or containing non-constants - build arrays with `Insert`.
- A server with an `init.c` compile error hangs rather than exits; `test.sh` detects this.
- `-missiontest` on the command line is the convention for "log state after 20 s and quit".
