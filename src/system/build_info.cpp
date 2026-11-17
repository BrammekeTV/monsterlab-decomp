// Build identification: date string and region, used for the crash screen header.
// Original object: .text 0x80014230 - 0x80014330
#include "types.h"

extern "C" void SysSprintf(char* buf, const char* fmt, ...);   // 0x80142EFC

static const char kBuildDate[] = "Aug 28 2008 (14:30)";       ORIG(0x803476A8)

enum { REGION_NA = 0, REGION_EU = 1 };

struct BuildRegion {
    u32 magic[3];     // 0xA1A1A1A1, 0xA2A2A2A2, 0xA3A3A3A3: patch markers
    s32 region;       // 0x0C
};
static BuildRegion sBuildRegion = {                            ORIG(0x80354868)
    { 0xA1A1A1A1, 0xA2A2A2A2, 0xA3A3A3A3 }, REGION_EU,
};

static const s32 kBuildNumber = 519;                           ORIG(0x805FC720)
static const s32 kBuildChange = 142835;                        ORIG(0x805FC724)

// 0x80014230
const char* GetBuildDate(void) {
    return kBuildDate;
}

// 0x8001425C
extern "C" s32 GetRegion(void) {
    return sBuildRegion.region;
}

// 0x8001426C
// e.g. "EU 519: Aug 28 2008 (14:30)@142835 (REL)"
void GetBuildString(char* out) {
    const char* region;
    switch (sBuildRegion.region) {
    case REGION_NA: region = "NA"; break;
    case REGION_EU: region = "EU"; break;
    default:        region = "??"; break;
    }
    SysSprintf(out, "%s %d: %s@%d (%s)", region, kBuildNumber, GetBuildDate(), kBuildChange, "REL");
}

// 0x80014328
s32 fn_80014328(void) {
    return 0;
}
