#!/bin/bash
# Build missions/<mission> into the server's mpmissions folder.
# The vanilla dayzOffline.<terrain> economy files are symlinked, then the
# mission's own files (init.c etc.) are copied over the top.
# The mission's persistence (storage_*) survives a redeploy, so a restart keeps bases,
# stashes and characters; WIPE=1 deletes it for a fresh start.
set -e
source "$(dirname "$0")/common.sh"
resolve_mission "$1"

VANILLA="$SERVER_DIR/mpmissions/dayzOffline.$TERRAIN"
TARGET="$SERVER_DIR/mpmissions/$MISSION"

if [ ! -d "$VANILLA" ]; then
	echo "vanilla mission not found: $VANILLA" >&2
	exit 1
fi
if [ "$MISSION" = "dayzOffline.$TERRAIN" ]; then
	echo "refusing to overwrite the vanilla mission" >&2
	exit 1
fi

KEEP=""
if [ -z "$WIPE" ] && ls -d "$TARGET"/storage_* >/dev/null 2>&1; then
	KEEP="$(mktemp -d "$SERVER_DIR/mpmissions/.keep.XXXXXX")"
	mv "$TARGET"/storage_* "$KEEP/"
fi
rm -rf "$TARGET"
mkdir -p "$TARGET"
if [ -n "$KEEP" ]; then
	mv "$KEEP"/storage_* "$TARGET/"
	rmdir "$KEEP"
	echo "kept persistence: $(for d in "$TARGET"/storage_*; do basename "$d"; done | tr '\n' ' ')"
elif [ -n "$WIPE" ]; then
	echo "wiped persistence"
fi

for f in "$VANILLA"/*; do
	case "$(basename "$f")" in
		init.c|storage_*) ;;
		*) ln -s "$f" "$TARGET/" ;;
	esac
done

# overlay: mission files replace the vanilla symlinks. A mission folder that vanilla also
# has (db/, env/...) is merged: the vanilla files are symlinked and the mission's copied
# over them, so a mission can override one economy file without carrying the whole set.
for f in "$PROJECT_DIR/missions/$MISSION"/*; do
	name="$(basename "$f")"
	case "$name" in mods.txt|mission.conf) continue ;; esac
	rm -rf "$TARGET/$name"
	if [ -d "$f" ] && [ -d "$VANILLA/$name" ]; then
		mkdir "$TARGET/$name"
		for v in "$VANILLA/$name"/*; do ln -s "$v" "$TARGET/$name/"; done
		for m in "$f"/*; do rm -f "$TARGET/$name/$(basename "$m")"; cp -r "$m" "$TARGET/$name/"; done
	else
		cp -r "$f" "$TARGET/"
	fi
done

# The engine can't #include relative to the mission folder, so lines of the form
#   #include "lib/Foo.c"
# in init.c are replaced here with the contents of that file from the project.
awk -v root="$PROJECT_DIR" '
	/^#include "lib\/[^"]+"/ {
		split($0, parts, "\"")
		file = root "/" parts[2]
		print "// ---- begin " parts[2]
		while ((getline line < file) > 0) print line
		close(file)
		print "// ---- end " parts[2]
		next
	}
	{ print }
' "$PROJECT_DIR/missions/$MISSION/init.c" > "$TARGET/init.c"

echo "deployed $MISSION -> $TARGET"
