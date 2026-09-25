#!/bin/bash
# Publish (or update) one of our mods on the Steam Workshop with SteamCMD.
#   tools/workshop_publish.sh <ModName> <steam login>
# Interactive: SteamCMD asks for the Steam password and a Steam Guard code, so run it
# yourself (in Claude Code: prefix the command with "!").
#
# First run creates the item (publishedfileid 0) and records the new id in
# mods/<ModName>/workshop.id; build_mod.py then writes meta.cpp with that id, and a second
# run uploads the mod with meta.cpp in it - the DayZ launcher and DZSA need meta.cpp to
# recognise the mod, so always run twice the first time. Later runs just update.
# Dependencies ("required items") can't be set from here: add them on the item's Workshop
# page afterwards (VehicleShootingAnywhere needs Vehicle Shooting + Survivor Animations).
# The "Mod" tag is essential: the DayZ launcher ignores untagged items ("waiting" forever).
# NOTE: logging in with SteamCMD knocks the desktop Steam client offline - don't publish
# while playing, and click Steam > Go Online afterwards.
set -e
source "$(dirname "$0")/common.sh"
NAME="$1"; LOGIN="$2"
[ -n "$NAME" ] && [ -n "$LOGIN" ] || { echo "usage: $0 <ModName> <steam login>" >&2; exit 1; }
[ -d "$PROJECT_DIR/mods/$NAME" ] || { echo "no such mod: mods/$NAME" >&2; exit 1; }

"$PROJECT_DIR/tools/build_mod.py" "$PROJECT_DIR/mods/$NAME" "$PROJECT_DIR/build"
[ -f "$PROJECT_DIR/build/@$NAME/keys/"*.bikey ] || { echo "the build is unsigned - create a key first (tools/sign_pbo.sh --create tarbaby)" >&2; exit 1; }

ID=0
[ -f "$PROJECT_DIR/mods/$NAME/workshop.id" ] && ID="$(cat "$PROJECT_DIR/mods/$NAME/workshop.id")"
DESC="$(cat "$PROJECT_DIR/mods/$NAME/workshop.txt" 2>/dev/null || echo "$NAME")"
PREVIEW="$PROJECT_DIR/mods/$NAME/preview.png"
[ -f "$PREVIEW" ] || PREVIEW=""

VDF="$PROJECT_DIR/build/$NAME.workshop.vdf"
cat > "$VDF" <<ITEM
"workshopitem"
{
	"appid"           "221100"
	"publishedfileid" "$ID"
	"contentfolder"   "$PROJECT_DIR/build/@$NAME"
	"previewfile"     "$PREVIEW"
	"visibility"      "0"
	"title"           "$NAME"
	"description"     "$DESC"
	"changenote"      "Uploaded with SteamCMD from $(git -C "$PROJECT_DIR" rev-parse --short HEAD 2>/dev/null)"
	"tags"
	{
		"0" "Mod"
		"1" "Vehicle"
		"2" "Mechanics"
	}
}
ITEM

echo "==> uploading build/@$NAME as Workshop item $ID (0 = new) - SteamCMD will ask for your password / Steam Guard code"
steamcmd +login "$LOGIN" +workshop_build_item "$VDF" +quit

NEW="$(grep -o '"publishedfileid" *"[0-9]*"' "$VDF" | grep -o '[0-9]*$')"
if [ -n "$NEW" ] && [ "$NEW" != "0" ] && [ "$NEW" != "$ID" ]; then
	echo "$NEW" > "$PROJECT_DIR/mods/$NAME/workshop.id"
	echo "==> created https://steamcommunity.com/sharedfiles/filedetails/?id=$NEW"
	echo "    now run this again so the upload includes meta.cpp with that id"
else
	echo "==> updated https://steamcommunity.com/sharedfiles/filedetails/?id=$ID"
fi
