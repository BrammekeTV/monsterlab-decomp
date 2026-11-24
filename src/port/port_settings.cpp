// PC platform layer: the settings file, monsterlab.ini next to the exe. It holds what the settings
// screen edits: display (gPortSettings), audio and the input bindings (gPortBindings). Loaded at
// start, before the command line (which wins); written when the settings screen saves.
//
//   [display]   width, height, aspect (16:9 or 4:3), stretch_ui, vsync, fullscreen, render_scale, texture_filter
//   [game]      path: the player's disc image or extracted folder
//   [audio]     volume (0-100), mute, remote_speaker (play the Wii Remote speaker sounds)
//   [mods]      NAME = 1 or 0, in load order (port_mods.cpp)
//   [controls]  NAME = key 1, key 2, mouse button, gamepad bits   (Windows virtual-key codes; 0 = none)
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/port.h"
#include "port/port_input.h"
#include "port/port_mods.h"

const char* PortSettingsPath(void) {
    static char path[MAX_PATH + 16];
    if (!path[0]) {
        DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
        char* slash = n && n < MAX_PATH ? strrchr(path, '\\') : NULL;
        if (slash) strcpy(slash + 1, "monsterlab.ini");
        else strcpy(path, "monsterlab.ini");
    }
    return path;
}

static char* Trim(char* s) {
    while (*s == ' ' || *s == '\t') s++;
    char* e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = 0;
    return s;
}

const char* const kPortFilterNames[PORT_FILTER_COUNT] = {"game", "nearest", "bilinear", "2x", "4x", "8x", "16x"};

static s32 Clamp(s32 v, s32 lo, s32 hi) { return v < lo ? lo : v > hi ? hi : v; }

BOOL PortSettingsLoad(const char* path, PortSettings* s) {
    FILE* f = fopen(path, "r");
    if (!f) return FALSE;
    char line[600], section[32] = "";
    while (fgets(line, sizeof(line), f)) {
        char* p = Trim(line);
        if (!*p || *p == ';' || *p == '#') continue;
        if (*p == '[') {
            char* end = strchr(p, ']');
            if (end) { *end = 0; snprintf(section, sizeof(section), "%s", p + 1); }
            continue;
        }
        char* eq = strchr(p, '=');
        if (!eq) continue;
        *eq = 0;
        char* key = Trim(p);
        char* val = Trim(eq + 1);
        if (!strcmp(section, "display")) {
            if (!strcmp(key, "width")) s->width = Clamp(atoi(val), 320, 8192);
            else if (!strcmp(key, "height")) s->height = Clamp(atoi(val), 240, 8192);
            else if (!strcmp(key, "aspect")) s->wide = strcmp(val, "4:3") != 0;
            else if (!strcmp(key, "stretch_ui")) s->stretchUi = atoi(val) != 0;
            else if (!strcmp(key, "vsync")) s->vsync = atoi(val) != 0;
            else if (!strcmp(key, "fullscreen")) s->fullscreen = atoi(val) != 0;
            else if (!strcmp(key, "windows_cursor")) s->windowsCursor = atoi(val) != 0;
            else if (!strcmp(key, "render_scale")) s->renderScale = Clamp(atoi(val), 50, 200);
            else if (!strcmp(key, "msaa")) {
                int n = atoi(val);
                s->msaa = n >= 8 ? 8 : n >= 4 ? 4 : n >= 2 ? 2 : 0;
            }
            else if (!strcmp(key, "fps_cap")) {
                if (!_stricmp(val, "off")) s->fpsCap = 0;
                else if (!_stricmp(val, "uncapped")) s->fpsCap = PORT_FPS_UNCAPPED;
                else if (!_stricmp(val, "display")) s->fpsCap = PORT_FPS_DISPLAY;
                else s->fpsCap = atoi(val) >= 10 ? Clamp(atoi(val), 10, 1000) : 0;
            }
            else if (!strcmp(key, "texture_filter")) {
                for (int k = 0; k < PORT_FILTER_COUNT; k++) if (!_stricmp(val, kPortFilterNames[k])) s->texFilter = k;
            }
        } else if (!strcmp(section, "mods")) {
            PortModsAdd(key, atoi(val) != 0);                  // the order of the lines is the load order
        } else if (!strcmp(section, "game")) {
            if (!strcmp(key, "path")) snprintf(s->gamePath, sizeof(s->gamePath), "%s", val);
        } else if (!strcmp(section, "audio")) {
            if (!strcmp(key, "volume")) s->volume = Clamp(atoi(val), 0, 100);
            else if (!strcmp(key, "mute")) s->mute = atoi(val) != 0;
            else if (!strcmp(key, "remote_speaker")) s->remoteSpeaker = atoi(val) != 0;
        } else if (!strcmp(section, "controls")) {
            for (int i = 0; i < gPortBindingCount; i++) {
                PortBinding* b = &gPortBindings[i];
                if (_stricmp(b->name, key)) continue;
                int k0 = 0, k1 = 0, mouse = 0;
                unsigned pad = 0;
                if (sscanf(val, "%d , %d , %d , %x", &k0, &k1, &mouse, &pad) >= 3) {
                    b->key[0] = Clamp(k0, 0, 255); b->key[1] = Clamp(k1, 0, 255); b->mouse = Clamp(mouse, 0, 2); b->pad = pad;
                }
                break;
            }
        }
    }
    fclose(f);
    return TRUE;
}

BOOL PortSettingsSave(const char* path, const PortSettings* s) {
    FILE* f = fopen(path, "w");
    if (!f) return FALSE;
    fprintf(f, "; Monster Lab PC port settings (written by the settings screen: F1 in the game)\n");
    fprintf(f, "[display]\nwidth = %d\nheight = %d\naspect = %s\nstretch_ui = %d\nvsync = %d\nfullscreen = %d\n",
            s->width, s->height, s->wide ? "16:9" : "4:3", s->stretchUi ? 1 : 0, s->vsync ? 1 : 0, s->fullscreen ? 1 : 0);
    fprintf(f, "; windows_cursor: 1 shows the Windows mouse cursor over the game as well as the game's own hand\nwindows_cursor = %d\n", s->windowsCursor ? 1 : 0);
    fprintf(f, "; internal frame buffer size, percent of the output picture (50-200); texture_filter: game, nearest, bilinear, 2x, 4x, 8x, 16x (anisotropic)\nrender_scale = %d\ntexture_filter = %s\n\n",
            s->renderScale, kPortFilterNames[s->texFilter >= 0 && s->texFilter < PORT_FILTER_COUNT ? s->texFilter : 0]);
    fprintf(f, "; msaa: off, 2, 4 or 8 samples on the frame buffer (applies at start)\nmsaa = %d\n", s->msaa);
    char cap[16];
    snprintf(cap, sizeof(cap), "%d", s->fpsCap);
    fprintf(f, "; fps_cap: off (a picture per game frame), 30, 60, 120, 144 or any rate, display (the monitor's rate), uncapped\nfps_cap = %s\n\n",
            s->fpsCap > 0 ? cap : s->fpsCap == PORT_FPS_UNCAPPED ? "uncapped" : s->fpsCap == PORT_FPS_DISPLAY ? "display" : "off");
    if (s->gamePath[0]) fprintf(f, "; your Monster Lab disc image (.iso, .wbfs, .rvz) or extracted folder\n[game]\npath = %s\n\n", s->gamePath);
    fprintf(f, "[audio]\nvolume = %d\nmute = %d\n; remote_speaker: also play the sounds the console sends to the Wii Remote's speaker\nremote_speaker = %d\n\n", s->volume, s->mute ? 1 : 0, s->remoteSpeaker ? 1 : 0);
    fprintf(f, "; NAME = key 1, key 2, mouse button (1 left, 2 right), gamepad bits (hex); keys are Windows virtual-key codes\n[controls]\n");
    for (int i = 0; i < gPortBindingCount; i++) {
        const PortBinding* b = &gPortBindings[i];
        fprintf(f, "%s = %d, %d, %d, %X\n", b->name, b->key[0], b->key[1], b->mouse, b->pad);
    }
    if (gPortModCount) fprintf(f, "\n; the mods folder's mods in load order (a later one wins); 1 = on, 0 = off\n[mods]\n");
    for (int i = 0; i < gPortModCount; i++) fprintf(f, "%s = %d\n", gPortMods[i].name, gPortMods[i].enabled ? 1 : 0);
    return fclose(f) == 0;
}

// Save, change everything, load: the file must bring every value back.
int PortSelfTestSettings(void) {
    char path[MAX_PATH + 40];
    DWORD n = GetTempPathA(MAX_PATH, path);
    if (!n || n >= MAX_PATH) { PortLog("selftest settings: no temporary folder"); return 1; }
    snprintf(path + n, sizeof(path) - n, "monsterlab_selftest_%lu.ini", GetCurrentProcessId());
    int fails = 0;
    PortBinding keep[32];
    int count = gPortBindingCount < 32 ? gPortBindingCount : 32;
    memcpy(keep, gPortBindings, sizeof(PortBinding) * count);

    PortSettings want = {};
    want.width = 2560; want.height = 1440; want.wide = FALSE; want.stretchUi = TRUE; want.vsync = TRUE; want.fullscreen = TRUE;
    want.volume = 37; want.mute = TRUE; want.renderScale = 150; want.texFilter = PORT_FILTER_AF8; want.fpsCap = PORT_FPS_UNCAPPED; want.msaa = 4; want.remoteSpeaker = FALSE;
    strcpy(want.gamePath, "D:\\My games\\Monster Lab (disc).iso");
    gPortBindings[0].key[0] = 'K'; gPortBindings[0].key[1] = 0; gPortBindings[0].mouse = 2; gPortBindings[0].pad = 0x20100;
    PortBinding changed = gPortBindings[0];
    if (!PortSettingsSave(path, &want)) { PortLog("selftest settings: cannot write %s", path); fails++; }
    memcpy(gPortBindings, keep, sizeof(PortBinding) * count);

    PortSettings got = {1920, 1080, TRUE, FALSE, FALSE, FALSE, 100, FALSE, "", 100, PORT_FILTER_GAME, 0, 0, TRUE};
    if (!fails && !PortSettingsLoad(path, &got)) { PortLog("selftest settings: cannot read %s", path); fails++; }
    if (!fails) {
        if (got.width != want.width || got.height != want.height || got.wide != want.wide || got.stretchUi != want.stretchUi ||
            got.vsync != want.vsync || got.fullscreen != want.fullscreen || got.volume != want.volume || got.mute != want.mute || got.renderScale != want.renderScale || got.texFilter != want.texFilter || got.fpsCap != want.fpsCap || got.msaa != want.msaa || got.remoteSpeaker != want.remoteSpeaker || strcmp(got.gamePath, want.gamePath)) {
            PortLog("selftest settings: read back %dx%d wide %d stretch %d vsync %d fullscreen %d volume %d mute %d",
                    got.width, got.height, got.wide, got.stretchUi, got.vsync, got.fullscreen, got.volume, got.mute);
            fails++;
        }
        const PortBinding* b = &gPortBindings[0];
        if (b->key[0] != changed.key[0] || b->key[1] != changed.key[1] || b->mouse != changed.mouse || b->pad != changed.pad) {
            PortLog("selftest settings: binding %s read back as %d, %d, %d, %X", b->name, b->key[0], b->key[1], b->mouse, b->pad);
            fails++;
        }
        for (int i = 1; i < count; i++)
            if (memcmp(&gPortBindings[i], &keep[i], sizeof(PortBinding))) { PortLog("selftest settings: binding %s changed", keep[i].name); fails++; }
    }
    // A missing file changes nothing, and values out of range are brought into range.
    FILE* f = fopen(path, "w");
    if (f) { fputs("[display]\nwidth = 5\nheight = 99999\nrender_scale = 900\n[audio]\nvolume = 250\n", f); fclose(f); }
    PortSettings odd = {1920, 1080, TRUE, FALSE, FALSE, FALSE, 100, FALSE, "", 100, PORT_FILTER_GAME, 0, 0, TRUE};
    PortSettingsLoad(path, &odd);
    if (odd.width != 320 || odd.height != 8192 || odd.volume != 100 || odd.renderScale != 200) { PortLog("selftest settings: out-of-range values gave %dx%d volume %d", odd.width, odd.height, odd.volume); fails++; }
    DeleteFileA(path);
    if (PortSettingsLoad(path, &odd)) { PortLog("selftest settings: a missing file was reported as loaded"); fails++; }
    memcpy(gPortBindings, keep, sizeof(PortBinding) * count);
    if (!fails) PortLog("selftest settings: ok (monsterlab.ini save and load)");
    return fails;
}
#endif
