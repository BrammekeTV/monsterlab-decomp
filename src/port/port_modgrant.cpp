// PC port: giving the player the parts that mods add (port_modpatch.h, docs/modding.md).
//
// A modded part is a row of the part table that nothing in the game hands out. Once a profile has a monster of its
// own (the save block's monster count, +0x10E8, goes from 0 to 1 when the first monster is charged in the Lightning
// Tower), every part of the enabled mods that this profile was not given yet goes into its inventory, tier 1 and
// full quality, and a notice names them. The same happens when a save that already has a monster is loaded and a
// new mod was added since. save.modparts (beside the saves) records each part given:
//     given|<save slot>|<scientist name>|<mod:id>
// so it is never given twice, also not after the player sold or scrapped it. Lines of this kind are ignored by the
// part-number reader (port_modpatch.cpp), which only takes "row|vanilla row|mod:id".
// [modparts] give = 0 in monsterlab.ini turns it off. PORT_MODPART_GIVE=1 (tests) gives the parts at once, also to
// a profile without a monster.
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "port/port.h"
#include "port/port_modpatch.h"
#include "port/port_overlay.h"
#include "game/part.h"

extern "C" {
void AddPartToPlayerInventory(u32 def, s32 tier, s32 quality);     // 0x802215FC
s32  SaveManager_GetCurrentSlot(void);                             // 0x801038B8
extern u8* gSaveSystem;                                            // 0x805FBBE0
}

namespace {

// Who the record is for: the save slot and the scientist's name (the save block starts with it), with the
// separators taken out.
std::string ProfileKey(s32 slot, const char* name) {
    std::string n;
    for (int i = 0; i < 32 && name[i]; i++) n += (name[i] == '|' || (u8)name[i] < ' ') ? '_' : name[i];
    char buf[16];
    snprintf(buf, sizeof(buf), "%d|", slot);
    return buf + n;
}

// The keys of the parts given to this profile, from the file.
std::vector<std::string> ReadGiven(const char* path, const std::string& profile) {
    std::vector<std::string> out;
    FILE* f = fopen(path, "rb");
    if (!f) return out;
    char line[512];
    const std::string head = "given|" + profile + "|";
    while (fgets(line, sizeof(line), f)) {
        size_t n = strlen(line);
        while (n && (u8)line[n - 1] <= ' ') line[--n] = 0;
        if (!strncmp(line, head.c_str(), head.size())) out.push_back(line + head.size());
    }
    fclose(f);
    return out;
}

BOOL AppendGiven(const char* path, const std::string& profile, const char* key) {
    std::string old;
    if (FILE* r = fopen(path, "rb")) {
        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), r)) > 0) old.append(buf, n);
        fclose(r);
    }
    FILE* f = fopen(path, "ab");
    if (!f) return FALSE;
    if (old.empty()) fputs("; Monster Lab PC port: which modded part each part number of the saves here is (row|vanilla row|mod:id).\n"
                           "; Keep this file with the saves.\n", f);
    else if (old.back() != '\n') fputc('\n', f);
    fprintf(f, "given|%s|%s\n", profile.c_str(), key);
    return fclose(f) == 0;
}

BOOL Has(const std::vector<std::string>& v, const char* key) {
    for (const std::string& s : v) if (!_stricmp(s.c_str(), key)) return TRUE;
    return FALSE;
}

BOOL Owned(s32 def) {
    u8* inv = gSaveSystem + 0xA30;
    PartInstance** parts = (PartInstance**)(inv + 0x28);
    s32 n = *(s32*)(inv + 0x6BC);
    for (s32 i = 0; i < n && i < 100; i++) if (parts[i] && parts[i]->defIndex == def) return TRUE;
    return FALSE;
}

// The part's name with the body slot it fills, as the notice says it: "Black Hole" -> "Black Hole Head". A mod's
// names are the game's names for the tiers, which leave the slot out.
std::string NoticeName(const char* name, s32 slot) {
    const char* word = slot == PART_SLOT_HEAD ? "Head" : slot == PART_SLOT_LEGS ? "Legs" :
                       slot == PART_SLOT_ARM ? "Arm" : slot == PART_SLOT_TORSO ? "Torso" : NULL;
    std::string n = name;
    if (!word) return n;
    const size_t len = strlen(word);
    if (n.size() >= len && !_stricmp(n.c_str() + n.size() - len, word)) return n;
    return n + " " + word;
}

std::string sProfile;                // the profile the list below is for
std::vector<std::string> sGiven;
s32 sChecked = -1000;                // frame of the last look (once a second is plenty)

}  // namespace

void PortModGrantFrame(void) {
    static int test = -1;
    if (test < 0) test = getenv("PORT_MODPART_GIVE") != NULL;
    if (!gPortSettings.giveModParts && !test) return;
    if (!PortModPartCount() || !gSaveSystem || !gSaveSystem[0]) return;      // no mod part, or no profile yet
    const s32 frame = PortFrameCount();
    if (frame - sChecked < 50 && frame >= sChecked) return;
    sChecked = frame;
    if (*(s32*)(gSaveSystem + 0x10E8) < 1 && !test) return;                    // no monster of their own yet
    const char* path = PortModPartsSidecar();
    const std::string profile = ProfileKey(SaveManager_GetCurrentSlot(), (const char*)gSaveSystem);
    const BOOL newProfile = profile != sProfile;
    if (newProfile) { sProfile = profile; sGiven = ReadGiven(path, profile); }
    std::string names;
    int given = 0;
    for (int i = 0; i < PortModPartCount(); i++) {
        const PortModPart* p = PortModPartAt(i);
        if (!p || p->defIndex < PORT_VANILLA_PARTS) continue;
        if (Has(sGiven, p->key)) {
            if (newProfile) PortLog("mods: part %s was given to profile %s before (in the inventory: %s)",
                                    p->key, profile.c_str(), Owned(p->defIndex) ? "yes" : "no");
            continue;
        }
        if (!Owned(p->defIndex)) {
            if (*(s32*)(gSaveSystem + 0xA30 + 0x6BC) >= 100) continue;       // inventory full: try again later
            AddPartToPlayerInventory((u32)p->defIndex, 1, 100);
            if (!names.empty()) names += ", ";
            PartDef def;
            PartDbGetDef(&def, gGameDb, p->defIndex);
            names += NoticeName(PortModPartName(i), def.slot);
            given++;
        }
        // Recorded also when it was owned already, so it is not given again once it is gone.
        if (!AppendGiven(path, profile, p->key)) PortLog("mods: cannot write %s", path);
        sGiven.push_back(p->key);
        PortLog("mods: part %s (row %d) given to profile %s", p->key, p->defIndex, profile.c_str());
    }
    if (given) PortNotice(given == 1 ? "New part unlocked from mods" : "New parts unlocked from mods", names.c_str(), 400);
}

// The record: written for one profile, read back for it only, other lines left alone.
int PortSelfTestModGrant(void) {
    char path[MAX_PATH + 40];
    DWORD n = GetTempPathA(MAX_PATH, path);
    if (!n || n >= MAX_PATH) { PortLog("selftest modgrant: no temporary folder"); return 1; }
    snprintf(path + n, sizeof(path) - n, "monsterlab_selftest_grant_%lu.txt", GetCurrentProcessId());
    DeleteFileA(path);
    int fails = 0;
    if (FILE* f = fopen(path, "wb")) { fputs("186|10|a:head\n", f); fclose(f); }
    const std::string bram = ProfileKey(0, "Bram"), other = ProfileKey(1, "A|b\n");
    if (other != "1|A_b_") { PortLog("selftest modgrant: profile key %s", other.c_str()); fails++; }
    if (!AppendGiven(path, bram, "a:head") || !AppendGiven(path, other, "b:arm")) { PortLog("selftest modgrant: cannot write"); fails++; }
    std::vector<std::string> g = ReadGiven(path, bram), h = ReadGiven(path, other), none = ReadGiven(path, ProfileKey(0, "Bra"));
    if (g.size() != 1 || g[0] != "a:head" || h.size() != 1 || h[0] != "b:arm" || !none.empty()) {
        PortLog("selftest modgrant: read back %u, %u, %u entries", (u32)g.size(), (u32)h.size(), (u32)none.size());
        fails++;
    }
    DeleteFileA(path);
    if (NoticeName("Black Hole", PART_SLOT_HEAD) != "Black Hole Head" || NoticeName("Iron arm", PART_SLOT_ARM) != "Iron arm" ||
        NoticeName("Spider", 3) != "Spider") {
        PortLog("selftest modgrant: notice names"); fails++;
    }
    if (!fails) PortLog("selftest modgrant: ok (given parts recorded per profile in save.modparts)");
    return fails;
}

#endif
