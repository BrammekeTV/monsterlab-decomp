// Small functions in 0x8014xxxx (online match connection state, disc/net error flags).
#include "types.h"
#include "revolution.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  OSLockMutex(u8* p);                                       // 0x8023A5B0
void  OSUnlockMutex(u8* p);                                       // 0x8023A68C
void  fn_802E9CBC(s32 counter);                                 // 0x802E9CBC
BOOL  fn_801F56FC(s32 flags);                                   // 0x801F56FC
void  fn_801F5714(s32 flags);                                   // 0x801F5714
void  fn_8000B964(u8* p, s32 a2);                               // 0x8000B964
BOOL  fn_80142358(void);                                        // 0x80142358
void  fn_80142470(void);                                        // 0x80142470
BOOL  fn_80147554(s32 button);                                  // 0x80147554
s32 fn_801FCA38(f32 x, f32 y);                                // 0x801FCA38
u32   fn_80140CE0(void);                                        // 0x80140CE0
BOOL  fn_8013E464(void);                                        // 0x8013E464
u8*   fn_80143158(u8* p);                                       // 0x80143158
u8*   fn_8014328C(u8* p, s32 del);                                    // 0x8014328C (global destructor)
void  __register_global_object(void* obj, void (*dtor)(void*), void* handle);
void  OSSleepTicks(s32 hi, u32 lo);                              // 0x8023D3C8
}

extern u8  lbl_80556AB0[];                  // 0x80556AB0 (connection lock object)
extern s32 lbl_805FBB48;                    // 0x805FBB48 (connection nesting counter)
extern s32 lbl_805FB418;                    // 0x805FB418 (connection state flags)
extern s32 lbl_805FBA30;                    // 0x805FBA30 (home button icon state)
extern f32 lbl_805FBA80;                    // 0x805FBA80
extern f32 lbl_806002D8;                    // 0x806002D8
extern s32 lbl_805FB970, lbl_805FB974;     // 0x805FB970 / 0x805FB974 (movie / session flags)
extern u8* lbl_805FB964;                    // 0x805FB964 (session object, vtable call)
extern s32 lbl_805FB438;                    // 0x805FB438 (pending response id)
extern u32 lbl_805F90E8[];                  // 0x805F90E8 (two screen-hash ids)
extern u8  lbl_803C4298[];                  // 0x803C4298 (level info block)
extern u8  lbl_803F22D8[];                  // 0x803F22D8 (global object)
extern u8  lbl_803F22C8[];                  // 0x803F22C8 (global object handle)
extern void (*lbl_803F2830[])(void);        // 0x803F2830 (callback table)
extern s32 lbl_805FB458;                    // 0x805FB458 (callback table count)

// 0x8014034C
// Ends the online match connection: takes the connection lock, bumps the nesting
// counter, shuts the link down, releases the lock, and decrements the counter.
void KillMatchConnection(void) {
    OSLockMutex(lbl_80556AB0);
    s32 count = lbl_805FBB48;
    lbl_805FBB48 = count + 1;
    fn_802E9CBC(count);
    OSUnlockMutex(lbl_80556AB0);
    lbl_805FBB48 = lbl_805FBB48 - 1;
}

// 0x801403CC
// Reports the inverse of the 0x8000 connection status flag (returns 1 when that flag is 0).
BOOL IsOpponentLoading(void) {
    return fn_801F56FC(0x8000) == 0;
}

// 0x80140408
// True when the opponent has closed the connection: flag 8 is set, the
// connection-state bit 2 is clear, and the session check passes.
BOOL DidOpponentCloseConnection(void) {
    if (fn_801F56FC(0x8) == 0) return 0;
    if (lbl_805FB418 & 0x2) return 0;
    return fn_80142358() != 0 ? 1 : 0;
}

// 0x80140458
// True while the network is stalled (flag 0x10000 is set).
u32 /*0x06*/ DidNetworkStall(void) {
    return fn_801F56FC(0x10000) != 0 ? 1 : 0;
}

// 0x801404A0
// True when our side lost the connection (flag 4 or flag 8) and the session check passes.
BOOL DidWeLoseConnection(void) {
    if (fn_801F56FC(0x4) == 0 && fn_801F56FC(0x8) == 0) return 0;
    return fn_80142358() != 0 ? 1 : 0;
}

// 0x801407C0
// Clears the lost-connection flag (4) and wipes the 16-byte level info block.
void ClearLostConnection(void) {
    fn_801F5714(0x4);
    fn_8000B964(lbl_803C4298, 0x10);
}

// 0x801407F4
// Clears the disc error flag (0x10) and wipes the 16-byte level info block.
void ClearDiscError(void) {
    fn_801F5714(0x10);
    fn_8000B964(lbl_803C4298, 0x10);
}

// 0x80140838
// True when the disc error flag (0x10) is set and the session check passes.
BOOL WasThereADiscError(void) {
    if (fn_801F56FC(0x10) == 0) return 0;
    return fn_80142358() != 0 ? 1 : 0;
}

// 0x8014087C
// True when the network error flag (0x40) is set and the session check passes.
BOOL WasThereANetError(void) {
    if (fn_801F56FC(0x40) == 0) return 0;
    return fn_80142358() != 0 ? 1 : 0;
}

// 0x801408C0
// Clears the network error flag (0x40), wipes the 16-byte level info block, then runs the follow-up step.
void ClearNetError(void) {
    fn_801F5714(0x40);
    fn_8000B964(lbl_803C4298, 0x10);
    fn_80142470();
}

// 0x80140A10
// Plays the home button icon feedback sound (button 0x108B) while the icon state is 1.
void HandleHomeButtonIcon(void) {
    if (lbl_805FBA30 != 1) return;
    if (fn_80147554(0x108B)) {
        fn_801FCA38(lbl_805FBA80, lbl_806002D8);
    }
}

// 0x80140A70
// True when we have connected to the opponent: the session object is set and its
// virtual check (slot 0x34) reports success.
BOOL DidWeConnectToOpponent(void) {
    s32 result = 0;
    if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != 0) {
        s32 ok = (*(s32 (**)(u8*))(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964);
        if (ok != 0) result = 1;
    }
    return result;
}

// 0x80140CE8
// Sends the pending response: signals the game flow with the stored response id
// (then clears it), or with the default id 0xCAA6159 when none is pending.
void SendResponse(void) {
    if (lbl_805FB438 != 0) {
        GameFlowSignal(lbl_805FB438, 0, 0);
        lbl_805FB438 = 0;
    } else {
        GameFlowSignal(0xCAA6159, 0, 0);
    }
}

// 0x80141C38
// Returns 1 if either of the two tracked UI screens is currently pushed, else 0.
u32 fn_80141C38(void) {
    for (s32 i = 0; i < 2; i++) {
        if (UIScreen_Is_Pushed(lbl_805F90E8[i]) == 1) return 1;
    }
    return 0;
}

// 0x80142080
// Returns 1 unless the match screen parts check (fn_8013E464) reports true.
BOOL NotStreamingMatchScreenParts(void) {
    return fn_8013E464() ? 0 : 1;
}

// 0x8014242C
// True when the value from fn_80140CE0 is the special alert box id 0x94B66F2D.
BOOL IsSpecialAlertBox(void) {
    return fn_80140CE0() == 0x94B66F2D ? 1 : 0;
}

// 0x801424B8
// Static initializer: sets up the global object and registers its destructor.
void fn_801424B8(void) {
    fn_80143158(lbl_803F22D8);
    __register_global_object(lbl_803F22D8, (void (*)(void*))fn_8014328C, lbl_803F22C8);
}

// 0x80142500
// Stores the current OS time (high word at +0, low word at +4) into the struct and returns it.
extern "C" u8* fn_80142500(u8* p) {
    OSTime t = OSGetTime();
    *(u32*)(p + 0x0) = (u32)((u64)t >> 32);
    *(u32*)(p + 0x4) = (u32)t;
    return p;
}

// 0x80142818
// Sleeps for a 64-bit microsecond count (hi:us): converts it to timebase ticks
// (ticks per microsecond = bus clock / 4 / 1000000, then divided by 8) and forwards the
// result to the sleep primitive.
extern "C" void SleepMicroseconds(s32 hi, u32 us) {
    u64 micros = ((u64)(u32)hi << 32) | us;
    u32 tpu = (u32)(((u64)(OSGetBusClock() >> 2) * 0x431BDE83u) >> 47);
    s64 ticks = (s64)(micros * tpu) / 8;
    OSSleepTicks((s32)(ticks >> 32), (u32)ticks);
}

extern "C" void fn_80142970(void);
// 0x80142970
// Calls every registered function in the table (count in lbl_805FB458), in order.
void fn_80142970(void) {
    for (s32 i = 0; i < lbl_805FB458; i++) {
        lbl_803F2830[i]();
    }
}
