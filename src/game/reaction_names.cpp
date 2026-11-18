// Animation names for fighters: builds the strings the character system looks animations up by
// (e.g. "legs_chimp_s_torso_humanoid_uppercut"). Original range 0x80215274-0x80215EF0 (partial).
#include <string.h>

#include "types.h"
#include "game/part.h"

struct TargetList5 { s32 limbs[5]; s32 count; };

extern "C" {
int   SysSprintf(char* buf, const char* fmt, ...);        // 0x80142EFC
u8**  FighterGetSlot(u8* f, s32 i);                       // 0x8020E794
void  TargetListInit(void* t);                            // 0x802161DC
void  TargetListDecode(void* t, const u32* mask, s32 side);   // 0x80216240
void* TargetListDestroy(void* t, s32 del);                // 0x80216200
}

// ---------------------------------------------------------------- name tables

// Animation set ("body type") names, indexed by PartDef::unk34.
static const char* const kAnimSetNames[21] = {
    "humanoid", "gun", "pincher", "pounder", "blade", "skinnybiped", "thickbiped",
    "backwardknee", "sharedhead", "medium", "bulky", "lean", "hunched", "floater", "feeler",
    "jboss", "vboss", "sboss", "gboss", "fboss", "mboss",
};

// Reaction / attack animation names, indexed by reaction id.
static const char* const kReactionNames[146] = {
    "Haymaker", "Hook", "Jab", "ChopOver", "OpenHand", "PinchHigh", "PinchLow", "Slap", "Smash",
    "SwipeLegs", "Uppercut", "ShootQuick", "ShootCharged", "Hose", "Mortar", "SpreadShot",
    "BlockArm", "SpellCast", "GroundPound", "Kick", "JumpKick", "Roundhouse", "Sweep", "Slide",
    "TLegLash", "TSpin", "TalonScrape", "LegBlock", "KickDust", "WalkForward", "Chomp",
    "HeadButt", "Breath", "Spit", "EyeLaser", "HeadBlock", "Recharge", "Sidekick", "Projectile",
    "Charge", "Flee", "Scream", "DodgeBack", "NisCharge", "InjureHeavy", "InjureLight",
    "DodgeDuck", "DodgeJump", "IdleReg", "IdleHurt", "IdleTired", "Stunned", "victory1",
    "Windandpow", "Bodyshake", "InjureSlam", "DirtScoop", "HugeJunkChunks", "MultipleMissiles",
    "ArmBlock", "StampedeCharge", "SpinRocks", "LegRancidGas", "LegSmokeSpew", "Roar", "Howl",
    "Gnaw", "SkullCrack", "BogBreath", "SmokeSpew", "SeaShower", "OilSplatter", "EyeLaserRed",
    "VenomSting", "Bonecrusher", "Slash", "QuickBite", "Stab", "MagnumDrop", "Backlash",
    "LightningStrikeSpell", "PutridBlastSpell", "CrystalShotSpell", "BlueLaserSpell",
    "OceanSurgeSpell", "QuickSting", "ElectricShock", "LightningStrike", "RancidGas",
    "OceanSurge", "CrystalShot", "ArmSqueeze", "KneeCrusher", "Buckshot", "BulletBurst",
    "SteamBlast", "Rake", "Shred", "PounceSmash", "TongueStrike", "ToadVenom", "Whack",
    "TailSwatLeft", "TailSwatRight", "CoffinClaw", "TelekenisisCoffin", "TombstoneCrush",
    "TelekenisisTombstone", "TelekenisisCruelStar", "Lacerate", "ThornGrater", "BatColony",
    "SonarScreech", "GraniteStompLeft", "GraniteStompRight", "Deathblow", "MercyStroke",
    "DivineWrath", "IronFist", "Trample", "Flay", "UppercutPincerBlade", "SlashPincer",
    "SlashKick", "PutridBlast", "AcidSurge", "OilSplatterBoss", "SmokeSpewBoss", "KnockoffHead",
    "KnockoffArm", "IdleIntro", "HeadBlock_Acidic", "HeadBlock_Celestial",
    "HeadBlock_Explosive", "HeadBlock_Radioactive", "LegBlock_Acidic", "LegBlock_Celestial",
    "LegBlock_Explosive", "LegBlock_Radioactive", "BlockArm_Acidic", "BlockArm_Celestial",
    "BlockArm_Explosive", "BlockArm_Radioactive", "HeadBlock_Fake", "LegBlock_Fake",
    "BlockArm_Fake",
};

// Limb names used inside animation names, indexed by limb (0 head, 1 legs, 2 left arm,
// 3 right arm, 4 torso).
static const char* const kLimbNames[5]  = { "head", "legs", "leftarm", "rightarm", "torso" };
static const char* const kLimbShort[5]  = { "hd", "lg", "la", "ra", "to" };
// Part definition used for a limb when the fighter has no part there.
static const s32 kDefaultPartDef[5] = { 0x48, 0x72, 0, 0, 0x97 };

static const char* const kOutOfRange = "out of range enumeration value";

extern "C" {

// 0x8020E310
const char* ReactionName(s32 id) {
    return (id >= 0 && id < 146) ? kReactionNames[id] : kOutOfRange;
}

// 0x8020E370
const char* AnimSetName(s32 id) {
    return (id >= 0 && id < 21) ? kAnimSetNames[id] : kOutOfRange;
}

// 0x8022128C
// Lower-case the ASCII letters of a string in place.
void StrToLower(char* s) {
    for (; *s != 0; s++) {
        if (*s > 0x40 && *s < 0x5B) *s += 0x20;
    }
}

// 0x802152FC
// Model key of a part: "p_<limb>_<animset>_<NNN>" where NNN is a number from the part definition.
void PartAnimKey(char* out, s32 limb, s32 defId) {
    if (defId == -1) {
        out[0] = 0;
        return;
    }
    strcpy(out, "p_");
    strcpy(out + 2, kLimbShort[limb]);
    out[4] = '_';
    PartDef def;
    PartDbGetDef(&def, gGameDb, defId);
    s32 set = def.unk34;
    // Two copies in the original: the second always wins (and writes "out" for an invalid set).
    strncpy(out + 5, set == -1 ? "UND" : AnimSetName(set), 3);
    strncpy(out + 5, AnimSetName(set), 3);
    SysSprintf(out + 8, "%03d", (s32)(s8)def.unk48[2]);
    out[0xB] = 0;
}

// 0x8021541C
// Build "<mainlimb>_<animset>_s_<limb>_<animset>_<reaction>" (lower-case reaction) for one limb
// of a fighter, e.g. the animation that limb plays. `fighter` is the fighter's part set: +0x00
// the main part, +0x04 the main limb index, +0x24 per-limb part holders. The first half is
// left out when the main limb is the limb itself. Empty string if a part has no animation set.
void ReactionBuildOne(char* out, s32 limb, void* fighter, const char* reaction) {
    if (fighter == NULL) {
        out[0] = 0;
        return;
    }
    u8* f = (u8*)fighter;
    char* p = out;
    s32 mainLimb = *(s32*)(f + 4);
    if (limb != mainLimb) {
        strcpy(out, kLimbNames[mainLimb]);
        p = out + strlen(kLimbNames[mainLimb]);
        *p++ = '_';
        s32 id = -1;
        if (*(s32**)f != NULL) id = **(s32**)f;
        if (id == -1) id = kDefaultPartDef[mainLimb];
        PartDef def;
        PartDbGetDef(&def, gGameDb, id);
        if (def.unk34 == -1) {
            out[0] = 0;
            return;
        }
        const char* name = AnimSetName(def.unk34);
        strcpy(p, name);
        p += strlen(name);
        *p++ = '_';
        *p++ = 's';
        *p++ = '_';
    }
    strcpy(p, kLimbNames[limb]);
    p += strlen(kLimbNames[limb]);
    *p++ = '_';
    s32 id = -1;
    s32** holder = (s32**)(*(void***)(f + 0x24))[limb];
    if (holder != NULL && *holder != NULL) id = **holder;
    if (id == -1) id = kDefaultPartDef[limb];
    PartDef def;
    PartDbGetDef(&def, gGameDb, id);
    if (def.unk34 == -1) {
        out[0] = 0;
        return;
    }
    const char* name = AnimSetName(def.unk34);
    strcpy(p, name);
    p += strlen(name);
    *p = '_';
    strcpy(p + 1, reaction);
    StrToLower(p + 1);
}

// 0x80215648
// Build the animation name of `limb` for each of the 5 fighters' part sets (0x100 bytes each).
void ReactionBuild(char* out, s32 limb, void** sets, const char* reaction) {
    for (s32 i = 0; i < 5; i++, out += 0x100) ReactionBuildOne(out, limb, sets[i], reaction);
}

// 0x80215EF0
// The "dodgeback" animation name of the torso for each of the 5 part sets.
void ReactionBuildDodgeBack(char* out, void** sets) {
    for (s32 i = 0; i < 5; i++, out += 0x100) ReactionBuildOne(out, 4, sets[i], "dodgeback");
}

// 0x8020E714
// Whether the fighter's main part has its own head animation set (not "sharedhead", set 8).
BOOL FighterHasOwnHeadAnims(void** fighter) {
    s32** set = (s32**)fighter[0];
    if (set == NULL) return FALSE;
    s32* part = set[0];
    if (part == NULL) return FALSE;
    PartDef def;
    PartDbGetDef(&def, gGameDb, *part);
    return def.unk34 != 8;
}

// 0x80212E28
// A limb slot is out of action (hp <= 0); a missing slot counts as out of action.
BOOL fn_80212E28(u8** slot) {
    if (slot == NULL) return TRUE;
    return **(s32**)slot <= 0;
}

// 0x80212E50
// A limb slot is at or below half health (twice hp <= max hp); a missing slot counts too.
BOOL fn_80212E50(u8** slot) {
    if (slot == NULL) return TRUE;
    s32* st = *(s32**)slot;
    return st[0] * 2 <= st[1];
}

// 0x802156B0
void ReactionBuildNamed(char* out, s32 limb, void* fighter, s32 reaction) {
    ReactionBuildOne(out, limb, fighter, ReactionName(reaction));
}

// 0x8021570C
// Same, for the reaction id of an action (row of the 0x14-byte table at db+0x430, `action`).
void fn_8021570C(char* out, s32 limb, void* fighter, s32 action) {
    PartAction a = (*(PartAction**)(gGameDb->data + 0x430))[action];
    ReactionBuildOne(out, limb, fighter, ReactionName(a.type));
}

// 0x802157A0
void ReactionBuildForAction(char* out, s32 limb, void** fighter, s32 action) {
    PartAction a = (*(PartAction**)(gGameDb->data + 0x430))[action];
    const char* name = ReactionName(a.type);
    for (s32 i = 0; i < 5; i++, out += 0x100) ReactionBuildOne(out, limb, fighter[i], name);
}

// 0x80215848
// Animation names for being hit by `round` (action at +0x2C, attacker limb at +0x34): the limb
// that was hit (the torso if it can't be told) plays "injureheavy" if it is at or below half
// health, else "injurelight".
void fn_80215848(char* out, u8* round, void** fighter) {
    PartAction a = (*(PartAction**)(gGameDb->data + 0x430))[*(s32*)(round + 0x2C)];
    TargetList5 t;
    TargetListInit(&t);
    u32 mask = a.unk08;
    TargetListDecode(&t, &mask, *(s32*)(round + 0x34));
    s32 limb = t.limbs[0];
    if (limb <= -1 || limb >= 5 || FighterHasOwnHeadAnims(fighter) || fighter == NULL ||
        fn_80212E28(FighterGetSlot((u8*)fighter, limb))) {
        limb = 4;
    }
    const char* name = fn_80212E50(FighterGetSlot((u8*)fighter, limb)) ? "injureheavy" : "injurelight";
    for (s32 i = 0; i < 5; i++, out += 0x100) ReactionBuildOne(out, limb, fighter[i], name);
    TargetListDestroy(&t, -1);
}

// 0x802159C4
// Idle animation after a hit: "idlehurt" if the legs are broken, "idletired" if the torso is at
// half health or less, else "idlereg"; played on the legs.
void ReactionBuildRecover(char* out, void** fighter) {
    const char* name = "idlereg";
    u8** legs = FighterGetSlot((u8*)fighter, 1);
    if (legs != NULL && fn_80212E28(legs)) {
        name = "idlehurt";
    } else {
        u8** torso = FighterGetSlot((u8*)fighter, 4);
        if (torso != NULL && fn_80212E50(torso)) name = "idletired";
    }
    for (s32 i = 0; i < 5; i++, out += 0x100) ReactionBuildOne(out, 1, fighter[i], name);
}

// 0x80215AE8
// Dodge animation names: "dodgeback" if the main part has its own head animations; otherwise
// "dodgejump" when the action targets the legs, else "dodgeduck". Played on the torso.
void fn_80215AE8(char* out, u8* round, void** fighter) {
    if (FighterHasOwnHeadAnims(fighter)) {
        for (s32 i = 0; i < 5; i++, out += 0x100) ReactionBuildOne(out, 4, fighter[i], "dodgeback");
        return;
    }
    PartAction a = (*(PartAction**)(gGameDb->data + 0x430))[*(s32*)(round + 0x2C)];
    TargetList5 t;
    TargetListInit(&t);
    u32 mask = a.unk08;
    TargetListDecode(&t, &mask, *(s32*)(round + 0x34));
    const char* name = (t.limbs[0] == 1) ? "dodgejump" : "dodgeduck";
    for (s32 i = 0; i < 5; i++, out += 0x100) ReactionBuildOne(out, 4, fighter[i], name);
    TargetListDestroy(&t, -1);
}

}  // extern "C"
