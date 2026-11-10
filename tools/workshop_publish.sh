#!/bin/bash
# Publish (or update) one of our mods on the Steam Workshop with SteamCMD.
#   tools/workshop_publish.sh <ModName> <steam login> [version]
# The version (e.g. v0.2) goes at the end of the description and leads the change note
# (the Workshop page's Change Notes tab), followed by the mod's git commit; Steam keeps no
# version of its own. Default: git describe --tags in the mod's repo, i.e. the tag itself
# on a tagged commit and v0.2-3-gabc1234 three commits after one; none in an untagged repo.
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
# Optional files in mods/<ModName>/: CHANGELOG.md (change notes, see below; not packed),
# workshop.txt (description), workshop.title (title,
# default the mod's name), workshop.tags (tags on one line, default Mod Vehicle Mechanics)
# and preview.png (under 1 MB).
# NOTE: logging in with SteamCMD knocks the desktop Steam client offline - don't publish
# while playing, and click Steam > Go Online afterwards.
set -e
source "$(dirname "$0")/common.sh"
NAME="$1"; LOGIN="$2"; VERSION="${3//\"/}"   # no double quotes inside a VDF string
[ -n "$NAME" ] && [ -n "$LOGIN" ] || { echo "usage: $0 <ModName> <steam login> [version]" >&2; exit 1; }
[ -d "$PROJECT_DIR/mods/$NAME" ] || { echo "no such mod: mods/$NAME" >&2; exit 1; }

"$PROJECT_DIR/tools/build_mod.py" "$PROJECT_DIR/mods/$NAME" "$PROJECT_DIR/build"
[ -f "$PROJECT_DIR/build/@$NAME/keys/"*.bikey ] || { echo "the build is unsigned - create a key first (tools/sign_pbo.sh --create tarbaby)" >&2; exit 1; }

ID=0
[ -f "$PROJECT_DIR/mods/$NAME/workshop.id" ] && ID="$(cat "$PROJECT_DIR/mods/$NAME/workshop.id")"
DESC="$(cat "$PROJECT_DIR/mods/$NAME/workshop.txt" 2>/dev/null || echo "$NAME")"
TITLE="$(cat "$PROJECT_DIR/mods/$NAME/workshop.title" 2>/dev/null || echo "$NAME")"
TAGS=($(cat "$PROJECT_DIR/mods/$NAME/workshop.tags" 2>/dev/null || echo Mod Vehicle Mechanics))
TAG_LINES=""
for i in "${!TAGS[@]}"; do
	TAG_LINES+="		\"$i\" \"${TAGS[$i]}\""$'\n'
done
# The commit and version of the repo the mod lives in (mods/<ModName> may link into
# another project)
COMMIT="$(git -C "$PROJECT_DIR/mods/$NAME/" rev-parse --short HEAD 2>/dev/null || true)"
# The change log: mods/<ModName>/CHANGELOG.md for a mod that shares this repo (its newest
# "## <version> - <date>" entry is then also the default version), else the CHANGELOG.md of
# the mod's own repo, with the version from its git tags.
REPO="$(git -C "$PROJECT_DIR/mods/$NAME/" rev-parse --show-toplevel 2>/dev/null || true)"
LOG="$PROJECT_DIR/mods/$NAME/CHANGELOG.md"
if [ -f "$LOG" ]; then
	[ -n "$VERSION" ] || VERSION="v$(awk '/^## /{ print $2; exit }' "$LOG")"
else
	LOG="$REPO/CHANGELOG.md"
fi
[ -n "$VERSION" ] || VERSION="$(git -C "$PROJECT_DIR/mods/$NAME/" describe --tags 2>/dev/null || true)"
[ -n "$VERSION" ] && DESC+=$'\n\n'"Version: $VERSION"
CHANGENOTE="Uploaded with SteamCMD${COMMIT:+ from $COMMIT}"
[ -n "$VERSION" ] && CHANGENOTE="$VERSION: $CHANGENOTE"
# With an entry for this version in the change log, that entry is the change note instead
if [ -n "$VERSION" ] && [ -f "$LOG" ]; then
	NOTES="$(awk -v v="${VERSION#v}" '/^## /{ on = ($2 == v); next } on' "$LOG" | sed '/./,$!d')"
	[ -n "$NOTES" ] && CHANGENOTE="$VERSION"$'\n\n'"$NOTES"
fi
PREVIEW="$PROJECT_DIR/mods/$NAME/preview.png"
[ -f "$PREVIEW" ] || PREVIEW=""

# The VDF has no usable escape for a double quote: one inside the description ends the
# string there and Steam keeps only what came before it. Turn them into single quotes.
case "$DESC$TITLE" in *\"*) echo "note: double quotes in the title/description become single quotes" ;; esac
DESC="${DESC//\"/\'}"; TITLE="${TITLE//\"/\'}"; CHANGENOTE="${CHANGENOTE//\"/\'}"
VDF="$PROJECT_DIR/build/$NAME.workshop.vdf"
cat > "$VDF" <<ITEM
"workshopitem"
{
	"appid"           "221100"
	"publishedfileid" "$ID"
	"contentfolder"   "$PROJECT_DIR/build/@$NAME"
	"previewfile"     "$PREVIEW"
	"visibility"      "0"
	"title"           "$TITLE"
	"description"     "$DESC"
	"changenote"      "$CHANGENOTE"
	"tags"
	{
$TAG_LINES	}
}
ITEM

echo "==> uploading build/@$NAME as Workshop item $ID (0 = new), change note: $CHANGENOTE"
echo "    SteamCMD will ask for your password / Steam Guard code"
steamcmd +login "$LOGIN" +workshop_build_item "$VDF" +quit

# SteamCMD rewrites the VDF with the new id, tab-separated
NEW="$(grep -o '"publishedfileid"[[:space:]]*"[0-9]*"' "$VDF" | grep -o '[0-9]*' | tail -n1)"
if [ -n "$NEW" ] && [ "$NEW" != "0" ] && [ "$NEW" != "$ID" ]; then
	echo "$NEW" > "$PROJECT_DIR/mods/$NAME/workshop.id"
	echo "==> created https://steamcommunity.com/sharedfiles/filedetails/?id=$NEW"
	echo "    now run this again so the upload includes meta.cpp with that id"
else
	echo "==> updated https://steamcommunity.com/sharedfiles/filedetails/?id=$ID"
fi
# SteamCMD can't set tags: they go on through the desktop client (back online first)
echo "==> tags: once Steam is back online, run tools/workshop_tags.py $NAME ${TAGS[*]}"
