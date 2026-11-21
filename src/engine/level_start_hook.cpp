// Hook called at the end of a level's start-up (fn_80007D68).
// Original: 0x801EB6E0 - 0x801EB704 (part of auto_03_801E73E4_text.s)
#include "types.h"

u8    lbl_805FB8F0;       ORIG(0x805FB8F0)   // cleared here; no other code in the game reads or writes it
void* lbl_805FB8F4;       ORIG(0x805FB8F4)   // optional callback; nothing in the retail game sets it
void* lbl_805FB8F8[2];    ORIG(0x805FB8F8)   // its argument (the symbol is 8 bytes)

// 0x801EB6E0
// Clears the flag and, if a callback is registered, calls it with its argument (a tail call in
// the original, so the callback's result would be this function's).
extern "C" void fn_801EB6E0(void) {
    lbl_805FB8F0 = 0;
    if (lbl_805FB8F4 != NULL) ((void (*)(void*))lbl_805FB8F4)(lbl_805FB8F8[0]);
}
