#!/bin/bash
# Build missions/<mission> into the server's mpmissions folder.
# The vanilla dayzOffline.<terrain> economy files are symlinked, then the
# mission's own files (init.c etc.) are copied over the top.
# Storage is wiped on every deploy so scripted spawns don't pile up.
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

rm -rf "$TARGET"
mkdir -p "$TARGET"

for f in "$VANILLA"/*; do
	case "$(basename "$f")" in
		init.c|storage_*) ;;
		*) ln -s "$f" "$TARGET/" ;;
	esac
done

# overlay: mission files replace the vanilla symlinks
for f in "$PROJECT_DIR/missions/$MISSION"/*; do
	rm -rf "$TARGET/$(basename "$f")"
	cp -r "$f" "$TARGET/"
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
