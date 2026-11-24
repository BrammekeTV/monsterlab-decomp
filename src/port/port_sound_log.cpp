// Sound request log (PORT_SOUND_LOG=1): one line per sound the game asks for, through PlaySound (by the
// sound group's name hash) or PlaySoundAt (a sound object: scripts, animation events). Each line says
// whether the sound was found, the sample it names (sound info +0x18), the zone of that sample, and
// whether the audio manager started it (a slot id) or why it gave up. Voice starts in the mixer are
// logged by PORT_AUDIO_TRACE (port_audio.cpp). Sounds kept in the voice records (fn_8020B5E8: entity and
// looping sounds, started once a record becomes audible) are logged as "record" when they start.
#include "port/port.h"
#include <stdlib.h>
#include <string.h>

extern "C" {
extern u8 gWorldState[];            // 0x803E6D00
extern u8 gAudio[];                 // 0x8047C7F0 (audio manager)
extern u8 lbl_805FBBB2[];           // 0x805FBBB2 (sound effects muted)
u8* fn_80208CE8(void);              // 0x80208CE8 (sound settings)
BOOL ZoneIsLoaded(void* world, s32 zone); // 0x8012F408
}

static s32 sOn = -1;
static u32 sHash;                   // PlaySound's group hash while it calls PlaySoundAt
static BOOL sInPlaySound;

static BOOL SoundLogOn(void) {
    if (sOn < 0) sOn = getenv("PORT_SOUND_LOG") != NULL;
    return sOn != 0;
}

// Why PlaySoundAt / the audio manager would not start the variation (the same checks, in order).
static const char* SoundReason(const u8* snd) {
    if (snd == NULL) return "no variation";
    const u8* info = *(u8* const*)snd;
    if (info == NULL) return "variation has no sound info";
    if (fn_80208CE8() == NULL || *(s32*)fn_80208CE8() == 0) return "sound settings off";
    if (*(u16*)(info + 0xC) != 0) return "positional (fn_802099B8)";
    if (lbl_805FBBB2[0] != 0) return "sound effects muted";
    if (!ZoneIsLoaded(gWorldState, *(s16*)(info + 0xE))) return "sample zone not loaded";
    if (gAudio[0x2B9] != 0) return "audio manager paused";
    if (*(void* const*)(info + 4) == NULL) return *(u32*)info == 0 ? "sample not loaded (load queued)" : "no sample data";
    return "slot table full or bad kind";
}

static void SoundLine(const char* via, u32 hash, const u8* snd, s32 result) {
    const u8* info = snd ? *(u8* const*)snd : NULL;
    char name[48] = "-";
    if (info != NULL) { memcpy(name, info + 0x18, sizeof name - 1); name[sizeof name - 1] = 0; }
    if (result != -1)
        PortLog("sound: frame %ld %s %08X found, \"%s\" zone %d: started, slot %08X", (long)PortFrameCount(), via, hash, name,
                info ? *(s16*)(info + 0xE) : -1, (u32)result);
    else
        PortLog("sound: frame %ld %s %08X %s, \"%s\" zone %d: not started (%s)", (long)PortFrameCount(), via, hash,
                snd ? "found" : "NOT FOUND", name, info ? *(s16*)(info + 0xE) : -1, SoundReason(snd));
}

extern "C" void PortSoundLogGroup(u32 hash, const void* group) {
    if (!SoundLogOn()) return;
    if (group == NULL) { SoundLine("PlaySound", hash, NULL, -1); return; }
    sHash = hash;
    sInPlaySound = TRUE;
}

extern "C" void PortSoundLogPlay(const void* snd, s32 result) {
    if (!SoundLogOn()) return;
    SoundLine(sInPlaySound ? "PlaySound" : "PlaySoundAt", sInPlaySound ? sHash : 0, (const u8*)snd, result);
    sInPlaySound = FALSE;
}

extern "C" void PortSoundLogRecord(const void* snd, s32 result) {
    if (!SoundLogOn()) return;
    SoundLine("record", 0, (const u8*)snd, result);
}
