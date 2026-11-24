// PC platform layer: the pieces of the Metrowerks C/C++ runtime the game code calls by name.
// The standard C functions (memcpy, strlen, sprintf, ...) come from the Windows C runtime.
// No original code. Design: docs/port_runtime.md.
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"

extern "C" {

// Registers a global object's destructor to run at exit. The PC build never unloads: nothing to do.
void __register_global_object(void* obj, void (*dtor)(void*), void* handle) {}

// Compiler helper: unsigned 64-bit integer to float.
f32 __cvt_ull_flt(u64 v) { return (f32)v; }

f64 ceil_(f64 x) { return ceil(x); }

// __construct_array: runs a constructor over an array in place. The original passes 1 as the
// constructor's second argument ("most derived object").
typedef void (*ArrayCtor)(void* obj, s32 flag);
void fn_8032F1E8(u8* arr, void (*ctor)(void), void (*dtor)(void*, s32), s32 size, s32 count) {
    for (s32 i = 0; i < count; i++) ((ArrayCtor)ctor)(arr + i * size, 1);
}

// __construct_new_array: the same for `new T[n]`. The block starts with a 16-byte header holding
// the element size and count; the array follows it.
void* fn_8032F028(void* mem, void* ctor, void* dtor, s32 size, s32 count) {
    if (!mem) return NULL;
    u8* arr = (u8*)mem + 0x10;
    *(s32*)mem = size;
    *(s32*)((u8*)mem + 4) = count;
    if (ctor) {
        for (s32 i = 0; i < count; i++) ((ArrayCtor)ctor)(arr + i * size, 1);
    }
    return arr;
}

// MSL strstr.
char* fn_80338BC0(char* s, const char* sub) {
    if (sub == NULL) return s;   // the original returns the string for a NULL pattern
    return strstr(s, sub);
}

// MSL sscanf.
s32 fn_8033861C(const char* s, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    s32 n = vsscanf(s, fmt, args);
    va_end(args);
    return n;
}

// MSL strncmp: returns the difference of the first two bytes that differ (unsigned), 0 if none.
s32 fn_80338A2C(const char* a, const char* b, u32 n) {
    for (; n != 0; n--, a++, b++) {
        u8 ca = (u8)*a, cb = (u8)*b;
        if (ca != cb) return (s32)ca - (s32)cb;
        if (ca == 0) break;
    }
    return 0;
}

// MSL localtime / gmtime: seconds -> broken-down time in the library's one static `struct tm`
// (0x805F5CA8: sec, min, hour, mday, mon, year, wday, yday, isdst), whose address is returned.
// With NULL the original fills in its zero date instead. GetRandomSeed is the caller.
void* fn_8033AE04(u32* seconds) {
    static s32* const tmOut = (s32*)(uintptr_t)0x805F5CA8;
    if (seconds == NULL) {
        static const s32 kZero[9] = {0, 0, 0, 1, 0, 0, 1, 0, -1};
        memcpy(tmOut, kZero, sizeof(kZero));
        tmOut[8] = 0;
        return tmOut;
    }
    time_t t = (time_t)*seconds;
    struct tm* r = gmtime(&t);
    if (r) {
        s32 v[9] = {r->tm_sec, r->tm_min, r->tm_hour, r->tm_mday, r->tm_mon, r->tm_year, r->tm_wday, r->tm_yday, 0};
        memcpy(tmOut, v, sizeof(v));
    }
    return tmOut;
}

// MSL strtok.
char* fn_80338A9C(char* s, const char* delims) { return strtok(s, delims); }

// MSL math (fdlibm). The parameter types are the ones the game's callers declare.
f64 fn_8033E8EC(f64 x) { return atan(x); }                  // atan
f64 fn_8033ED6C(f64 x) { return floor(x); }                 // floor
f64 fn_8033F17C(f64 x) { return tan(x); }                   // tan
f64 fn_8033F1F4(f64 x) { return acos(x); }                  // acos
f64 fn_8033F1F8(f64 x) { return asin(x); }                  // asin
f32 fn_8033F1FC(f32 y, f32 x) { return (f32)atan2((f64)y, (f64)x); }   // atan2
f32 fn_8033F200(f32 x, f32 y) { return (f32)fmod((f64)x, (f64)y); }    // fmod
f64 fn_8033F204(f64 x) { return log(x); }                   // log
f64 fn_8033F20C(f64 x, f64 y) { return pow(x, y); }         // pow
f64 fn_8033F45C(f64 x) { return sqrt(x); }                  // sqrt

// __cvt_dbl_ll (0x8032FB38): a double to a signed 64-bit integer, truncated toward zero, 0 below 1 in
// magnitude, saturated at +-2^63 (and for NaN by its sign). The game's callers pass the f32 as the
// second argument and a clock they do not use in the first (r3 is not read by the original).
u64 fn_8032FB38(u32 /*unused*/, f32 secs) {
    f64 x = (f64)secs;
    bool neg = signbit(x) != 0;
    if (x != x) return neg ? 0x8000000000000000ull : 0x7FFFFFFFFFFFFFFFull;
    if (fabs(x) < 1.0) return 0;
    if (x >= 9223372036854775808.0) return 0x7FFFFFFFFFFFFFFFull;
    if (x <= -9223372036854775808.0) return 0x8000000000000000ull;
    return (u64)(s64)x;
}

// MSL qsort.
void fn_80336FE0(void* base, s32 n, s32 size, void* cmp) {
    qsort(base, (size_t)n, (size_t)size, (int (__cdecl*)(const void*, const void*))cmp);
}

// MSL time: seconds; also stored through the argument when it is not NULL.
s32 fn_8033ADCC(s32* out) {
    s32 t = (s32)time(NULL);
    if (out) *out = t;
    return t;
}

// A thunk to the library's strncmp.
s32 fn_8033B1B8(const char* a, const char* b, s32 n) { return fn_80338A2C(a, b, (u32)n); }

// MSL stricmp: compares the lower-case forms as signed chars (bytes of 0x80 and up are left as
// they are) and returns -1, 0 or 1.
s32 fn_8033B1C0(const char* a, const char* b) {
    for (;;) {
        s32 ca = (s8)*a++;
        s32 cb = (s8)*b++;
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca < cb) return -1;
        if (ca > cb) return 1;
        if (ca == 0) return 0;
    }
}

}  // extern "C"
