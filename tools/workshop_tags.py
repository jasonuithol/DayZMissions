#!/usr/bin/env python3
"""Set the tags of one of our Workshop items through the running, logged-in Steam client.
SteamCMD's workshop_build_item can't set tags, and the DayZ launcher ignores untagged
items. Uses the flat Steamworks API from the DayZ server's libsteam_api.so via ctypes.

    tools/workshop_tags.py <ModName> [Tag ...]      (default tags: Mod Vehicle Mechanics)
"""
import ctypes, os, sys, tempfile, time

APP = 221100
here = os.path.dirname(os.path.abspath(__file__))
project = os.path.dirname(here)
steam = os.environ.get('STEAM', os.path.expanduser('~/.steam/debian-installation'))
lib_path = os.path.join(steam, 'steamapps/common/DayZServer/libsteam_api.so')

name = sys.argv[1]
tags = sys.argv[2:] or ['Mod', 'Vehicle', 'Mechanics']
item = int(open(os.path.join(project, 'mods', name, 'workshop.id')).read().strip())

# SteamAPI_Init reads steam_appid.txt from the working directory
work = tempfile.mkdtemp()
open(os.path.join(work, 'steam_appid.txt'), 'w').write(str(APP))
os.chdir(work)

api = ctypes.CDLL(lib_path)
api.SteamAPI_ManualDispatch_Init()
api.SteamAPI_Init.restype = ctypes.c_bool
if not api.SteamAPI_Init():
    sys.exit('SteamAPI_Init failed: is the Steam client running and online?')

api.SteamAPI_SteamUGC_v017.restype = ctypes.c_void_p
ugc = api.SteamAPI_SteamUGC_v017()
api.SteamAPI_ISteamUGC_StartItemUpdate.restype = ctypes.c_uint64
api.SteamAPI_ISteamUGC_StartItemUpdate.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint64]
handle = api.SteamAPI_ISteamUGC_StartItemUpdate(ugc, APP, item)

class StringArray(ctypes.Structure):
    _fields_ = [('strings', ctypes.POINTER(ctypes.c_char_p)), ('count', ctypes.c_int32)]
c_tags = (ctypes.c_char_p * len(tags))(*[t.encode() for t in tags])
arr = StringArray(ctypes.cast(c_tags, ctypes.POINTER(ctypes.c_char_p)), len(tags))
api.SteamAPI_ISteamUGC_SetItemTags.restype = ctypes.c_bool
api.SteamAPI_ISteamUGC_SetItemTags.argtypes = [ctypes.c_void_p, ctypes.c_uint64, ctypes.POINTER(StringArray), ctypes.c_bool]
if not api.SteamAPI_ISteamUGC_SetItemTags(ugc, handle, ctypes.byref(arr), False):
    sys.exit('SetItemTags refused')

api.SteamAPI_ISteamUGC_SubmitItemUpdate.restype = ctypes.c_uint64
api.SteamAPI_ISteamUGC_SubmitItemUpdate.argtypes = [ctypes.c_void_p, ctypes.c_uint64, ctypes.c_char_p]
call = api.SteamAPI_ISteamUGC_SubmitItemUpdate(ugc, handle, b'tags: ' + ', '.join(tags).encode())

# The update is async. Steam's callback plumbing is awkward from ctypes, so wait for the
# upload handle to finish, then confirm the tags through the public Web API.
import json, urllib.parse, urllib.request
api.SteamAPI_ISteamUGC_GetItemUpdateProgress.restype = ctypes.c_int
api.SteamAPI_ISteamUGC_GetItemUpdateProgress.argtypes = [ctypes.c_void_p, ctypes.c_uint64, ctypes.POINTER(ctypes.c_uint64), ctypes.POINTER(ctypes.c_uint64)]
api.SteamAPI_RunCallbacks()
deadline = time.time() + 120
while time.time() < deadline:
    api.SteamAPI_RunCallbacks()
    done_b = ctypes.c_uint64(); total_b = ctypes.c_uint64()
    if api.SteamAPI_ISteamUGC_GetItemUpdateProgress(ugc, handle, ctypes.byref(done_b), ctypes.byref(total_b)) == 0:
        break
    time.sleep(0.5)
time.sleep(3)
api.SteamAPI_Shutdown()

def item_tags():
    data = urllib.parse.urlencode({'itemcount': 1, 'publishedfileids[0]': item}).encode()
    with urllib.request.urlopen('https://api.steampowered.com/ISteamRemoteStorage/GetPublishedFileDetails/v1/', data, timeout=30) as r:
        return [t['tag'] for t in json.load(r)['response']['publishedfiledetails'][0].get('tags', [])]

deadline = time.time() + 60
while time.time() < deadline:
    have = item_tags()
    if all(t in have for t in tags):
        print('tags on item %d: %s' % (item, ', '.join(have)))
        sys.exit(0)
    time.sleep(5)
sys.exit('Steam accepted the update but the tags have not shown up: is the Steam client online?')
