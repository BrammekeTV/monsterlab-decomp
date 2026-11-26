// PC audio: the effects on AX's aux buses (AXFX): delay, chorus and the high-quality reverb.
// No original code. The record layouts (which float is which parameter) were read from the SDK
// code in main.dol; the signal processing is written from the effects' published behaviour
// (a feedback delay, a modulated-delay chorus, a pre-delay + early reflections + comb/allpass
// late reverb), so it sounds like those effects without being sample-identical to the DSP code.
//
// The game keeps one record per effect and bus (lbl_804FDAA0 + 0x20000 + ...), fills the
// parameters, calls the effect's init and registers the callback on the aux bus. AX then calls the
// callback once per AX frame with the bus' three buffers (left, right, surround; 96 s32 each); the
// effect replaces them with its wet output, which AX mixes back into the main output.
// The state of an effect (its delay lines) is kept here, keyed by the record's address.
#ifdef _WIN32
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <map>

#include "port/port.h"

#define FRAME  96
#define RATE   32000.0f
#define SAMPLES_PER_MS 32.0f

static inline f32  PF(const u8* r, u32 off) { return *(const f32*)(r + off); }
static inline f32  Clamp01(f32 x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }

struct Fx { int kind = 0; virtual ~Fx() {} virtual void Process(s32** bus) = 0; };
// (a function-local static: the self-test can run during static initialisation)
static std::map<const u8*, Fx*>& FxMap() { static std::map<const u8*, Fx*> m; return m; }
#define sFx FxMap()

static void FxDestroy(const u8* r) {
    auto it = sFx.find(r);
    if (it == sFx.end()) return;
    delete it->second;
    sFx.erase(it);
}

// ---------------------------------------------------------------- delay
// Record: +0x38 longest delay (ms), +0x3C delay (ms), +0x40 feedback, +0x44 low-pass (1 = none),
// +0x50 output level. Per channel a delay line; the input is low-passed into the line, the line's
// output (times the output level) is what goes back on the bus.
struct DelayFx : Fx {
    std::vector<s32> line[3];
    s32 size = 1, idx = 0, st[3] = {0, 0, 0}, fb = 0, lp = 0, out = 0;
    void Process(s32** bus) override {
        for (int n = 0; n < FRAME; n++) {
            for (int c = 0; c < 3; c++) {
                s32 d = line[c][idx];
                st[c] = (s32)(((s64)(128 - lp) * bus[c][n] + (s64)lp * st[c]) >> 7);
                line[c][idx] = st[c] + (s32)(((s64)d * fb) >> 7);
                bus[c][n] = (s32)(((s64)d * out) >> 7);
            }
            if (++idx >= size) idx = 0;
        }
    }
};

static f32 sDelayMaxLpWeight = 0.9999f;

static s32 DelayCreate(const u8* r) {
    FxDestroy(r);
    f32 maxMs = PF(r, 0x38), ms = PF(r, 0x3C);
    if (!(maxMs > 0.0f) || ms > maxMs || ms < 0.0f) return 0;
    f32 fb = PF(r, 0x40), lp = PF(r, 0x44), out = PF(r, 0x50);
    if (fb < 0.0f || fb > 1.0f || lp < 0.0f || lp > 1.0f || out < 0.0f || out > 1.0f) return 0;
    DelayFx* d = new DelayFx();
    d->kind = 1;
    d->size = (s32)(ms * SAMPLES_PER_MS);
    if (d->size < 1) d->size = 1;
    for (int c = 0; c < 3; c++) d->line[c].assign(d->size, 0);
    d->fb = (s32)(128.0f * fb);
    f32 prev = 1.0f - lp;
    if (prev > sDelayMaxLpWeight) prev = sDelayMaxLpWeight;
    d->lp = (s32)(128.0f * prev);
    d->out = (s32)(128.0f * out);
    sFx[r] = d;
    return 1;
}

// ---------------------------------------------------------------- chorus
// Record: +0x80 base delay (ms), +0x84 modulation depth (fraction of the base delay), +0x88 period
// of the modulation (s), +0x8C feedback, +0x98 output level. The read position of a delay line
// swings around the base delay; the wet signal is the output and, times the feedback, goes back
// into the line.
struct ChorusFx : Fx {
    std::vector<f32> line[3];
    s32 len = 1, w = 0;
    f32 base = 1, depth = 0, fb = 0, out = 1;
    double phase = 0, step = 0;
    void Process(s32** bus) override {
        for (int n = 0; n < FRAME; n++) {
            f32 d = base * (1.0f + depth * (f32)sin(phase * 6.283185307179586));
            if (d < 2.0f) d = 2.0f;
            phase += step; if (phase >= 1.0) phase -= 1.0;
            f32 pos = (f32)w - d;
            if (pos < 0.0f) pos += (f32)len;
            s32 i0 = (s32)pos; f32 fr = pos - (f32)i0;
            s32 i1 = i0 + 1 >= len ? 0 : i0 + 1;
            for (int c = 0; c < 3; c++) {
                f32 wet = line[c][i0] + (line[c][i1] - line[c][i0]) * fr;
                line[c][w] = (f32)bus[c][n] + wet * fb;
                bus[c][n] = (s32)(wet * out);
            }
            if (++w >= len) w = 0;
        }
    }
};

static s32 ChorusCreate(const u8* r) {
    FxDestroy(r);
    f32 baseMs = PF(r, 0x80), depth = PF(r, 0x84), period = PF(r, 0x88), fb = PF(r, 0x8C), out = PF(r, 0x98);
    if (!(baseMs > 0.0f) || depth < 0.0f || depth > 1.0f || !(period > 0.0f) || fb < 0.0f || fb > 1.0f || out < 0.0f || out > 1.0f) return 0;
    ChorusFx* c = new ChorusFx();
    c->kind = 2;
    c->base = baseMs * SAMPLES_PER_MS;
    c->depth = depth;
    c->fb = fb > 0.95f ? 0.95f : fb;
    c->out = out;
    c->step = 1.0 / (period * RATE);
    c->len = (s32)(c->base * (1.0f + depth)) + 4;
    for (int i = 0; i < 3; i++) c->line[i].assign(c->len, 0.0f);
    sFx[r] = c;
    return 1;
}

// ---------------------------------------------------------------- reverb
// Record (HiExp): +0x110 early reflection set 0..7, +0x114 longest pre-delay (s), +0x118 pre-delay (s),
// +0x11C late reverb set 0..5, +0x120 reverb time (s), +0x124 coloration, +0x128 damping, +0x12C crosstalk,
// +0x130 early reflection level, +0x134 late reverb level, +0x140 output level.
// Pre-delay, then six early reflection taps (spacing grows with the set) and a late reverb of four
// feedback combs (damped, decay set by the time) and two allpasses; the set picks the density.
struct Comb { std::vector<f32> b; s32 i = 0; f32 lpState = 0, g = 0; };
struct Allpass { std::vector<f32> b; s32 i = 0; f32 g = 0.5f; };
struct ReverbFx : Fx {
    std::vector<f32> pre[3];  s32 preLen = 1, preIdx = 0;
    std::vector<f32> early[3]; s32 earlyLen = 1, earlyIdx = 0; s32 tap[3][6]; f32 tapGain[6];
    Comb comb[3][4]; Allpass ap[3][2];
    f32 damp = 0, xt = 0, earlyLvl = 0, lateLvl = 0, out = 1;
    void Process(s32** bus) override {
        f32 res[3][FRAME];
        for (int n = 0; n < FRAME; n++) {
            for (int c = 0; c < 3; c++) {
                f32 x = (f32)bus[c][n];
                // pre-delay
                f32 p = pre[c][preIdx];
                pre[c][preIdx] = x;
                // early reflections
                early[c][earlyIdx] = p;
                f32 e = 0;
                for (int t = 0; t < 6; t++) {
                    s32 k = earlyIdx - tap[c][t]; if (k < 0) k += earlyLen;
                    e += early[c][k] * tapGain[t];
                }
                // late reverb: the pre-delayed input into four combs
                f32 late = 0;
                for (int m = 0; m < 4; m++) {
                    Comb& cb = comb[c][m];
                    f32 y = cb.b[cb.i];
                    cb.lpState = y * (1.0f - damp) + cb.lpState * damp;
                    cb.b[cb.i] = p * 0.25f + cb.lpState * cb.g;
                    if (++cb.i >= (s32)cb.b.size()) cb.i = 0;
                    late += y;
                }
                for (int m = 0; m < 2; m++) {
                    Allpass& a = ap[c][m];
                    f32 y = a.b[a.i];
                    f32 v = late + y * a.g;
                    a.b[a.i] = v;
                    late = y - v * a.g;
                    if (++a.i >= (s32)a.b.size()) a.i = 0;
                }
                res[c][n] = (e * 0.5f * earlyLvl + late * 0.7f * lateLvl) * out;
            }
            if (++preIdx >= preLen) preIdx = 0;
            if (++earlyIdx >= earlyLen) earlyIdx = 0;
        }
        for (int n = 0; n < FRAME; n++) {
            f32 l = res[0][n], r = res[1][n];
            bus[0][n] = (s32)(l * (1.0f - 0.5f * xt) + r * 0.5f * xt);
            bus[1][n] = (s32)(r * (1.0f - 0.5f * xt) + l * 0.5f * xt);
            bus[2][n] = (s32)res[2][n];
        }
    }
};

static s32 ReverbCreate(const u8* r) {
    FxDestroy(r);
    u32 early = *(const u32*)(r + 0x110), lateSet = *(const u32*)(r + 0x11C);
    f32 preMax = PF(r, 0x114), pre = PF(r, 0x118), time = PF(r, 0x120);
    f32 col = PF(r, 0x124), damp = PF(r, 0x128), xt = PF(r, 0x12C), eg = PF(r, 0x130), lg = PF(r, 0x134), og = PF(r, 0x140);
    if (early >= 8 || lateSet >= 6 || !(preMax >= 0.0f) || pre < 0.0f || pre > preMax || time < 0.0f) return 0;
    f32 lv[] = {col, damp, xt, eg, lg, og};
    for (f32 x : lv) if (x < 0.0f || x > 1.0f) return 0;
    ReverbFx* v = new ReverbFx();
    v->kind = 3;
    v->preLen = (s32)(pre * RATE) + 1;
    v->damp = Clamp01(damp) * 0.8f; v->xt = xt; v->earlyLvl = eg; v->lateLvl = lg; v->out = og;
    static const f32 kTapMs[6] = {1.0f, 1.7f, 2.6f, 3.9f, 5.3f, 7.1f};
    static const f32 kDensity[6] = {1.0f, 1.15f, 1.3f, 0.85f, 1.5f, 0.7f};
    f32 spread = 3.0f + 2.0f * (f32)early;                              // ms of the first tap
    v->earlyLen = (s32)(spread * 7.1f * 1.5f * SAMPLES_PER_MS) + 8;
    for (int t = 0; t < 6; t++) v->tapGain[t] = (t & 1 ? -0.7f : 0.8f) * powf(0.75f, (f32)t);
    static const f32 kCombMs[4] = {29.7f, 37.1f, 41.1f, 43.7f};
    f32 rt = time < 0.05f ? 0.05f : time;
    for (int c = 0; c < 3; c++) {
        v->pre[c].assign(v->preLen, 0.0f);
        v->early[c].assign(v->earlyLen, 0.0f);
        for (int t = 0; t < 6; t++) v->tap[c][t] = 1 + (s32)(spread * kTapMs[t] * (1.0f + 0.13f * (f32)c) * SAMPLES_PER_MS);
        for (int m = 0; m < 4; m++) {
            s32 len = (s32)((kCombMs[m] * kDensity[lateSet] + 0.7f * (f32)c) * SAMPLES_PER_MS);
            v->comb[c][m].b.assign(len, 0.0f);
            v->comb[c][m].g = powf(10.0f, -3.0f * (f32)len / (rt * RATE));
        }
        static const f32 kApMs[2] = {5.0f, 1.7f};
        for (int m = 0; m < 2; m++) {
            v->ap[c][m].b.assign((s32)(kApMs[m] * SAMPLES_PER_MS) + c, 0.0f);
            v->ap[c][m].g = 0.4f + 0.3f * col;
        }
    }
    sFx[r] = v;
    return 1;
}

// ---------------------------------------------------------------- the SDK's entry points
extern "C" {
// init / settings (take the parameters again) / shutdown / aux callback, per effect
void fn_8025D2E0(u8* r) { DelayCreate(r); }
void fn_8025D5A4(u8* r) { DelayCreate(r); }
void fn_8025D724(u8* r) { FxDestroy(r); }
void fn_8025D7D0(void* data, u8* r) {
    auto it = sFx.find(r);
    if (it == sFx.end() && PortStateLoaded()) { DelayCreate(r); it = sFx.find(r); }   // a loaded state: the effect starts again from silence
    if (it != sFx.end()) it->second->Process((s32**)data);
}
void fn_8025E3E4(u8* r) { ChorusCreate(r); }
void fn_8025E640(u8* r) { ChorusCreate(r); }
void fn_8025E768(u8* r) { FxDestroy(r); }
void fn_8025E7F8(void* data, u8* r) {
    auto it = sFx.find(r);
    if (it == sFx.end() && PortStateLoaded()) { ChorusCreate(r); it = sFx.find(r); }   // a loaded state: the effect starts again from silence
    if (it != sFx.end()) it->second->Process((s32**)data);
}
void AXFXReverbHiExpInit(u8* r) { ReverbCreate(r); }
void fn_8025B65C(void* r) { ReverbCreate((u8*)r); }
void AXFXReverbHiExpShutdown(void* r) { FxDestroy((u8*)r); }
void fn_8025B780(void* data, u8* r) {
    auto it = sFx.find(r);
    if (it == sFx.end() && PortStateLoaded()) { ReverbCreate(r); it = sFx.find(r); }   // a loaded state: the effect starts again from silence
    if (it != sFx.end()) it->second->Process((s32**)data);
}
}

// ---------------------------------------------------------------- self-test
static f32 Energy(const std::vector<s32>& v, size_t a, size_t b) {
    double e = 0;
    for (size_t i = a; i < b && i < v.size(); i++) e += (double)v[i] * v[i];
    return (f32)e;
}

// Runs an effect over n frames: an impulse of 10000 on the left of the first frame; returns the left output.
static std::vector<s32> RunImpulse(void (*cb)(void*, u8*), u8* rec, int frames) {
    std::vector<s32> out;
    s32 b[3][FRAME];
    for (int f = 0; f < frames; f++) {
        memset(b, 0, sizeof(b));
        if (f == 0) b[0][0] = 10000;
        s32* bus[3] = {b[0], b[1], b[2]};
        cb(bus, rec);
        out.insert(out.end(), b[0], b[0] + FRAME);
    }
    return out;
}

int PortSelfTestAxfx(void) {
    int fails = 0;
#define CHECK(cond) do { if (!(cond)) { fails++; PortLog("self-test FAILED: %s (line %d)", #cond, __LINE__); } } while (0)
    static u8 rec[0x400];
    // Delay: 10 ms (320 samples), feedback 50 %, no low-pass, full output.
    memset(rec, 0, sizeof(rec));
    *(f32*)(rec + 0x38) = 20.0f; *(f32*)(rec + 0x3C) = 10.0f; *(f32*)(rec + 0x40) = 0.5f; *(f32*)(rec + 0x44) = 1.0f; *(f32*)(rec + 0x50) = 1.0f;
    fn_8025D2E0(rec);
    std::vector<s32> d = RunImpulse(fn_8025D7D0, rec, 12);
    CHECK(d[0] == 0 && d[319] == 0 && d[320] > 9900 && d[320] <= 10000);
    CHECK(d[640] > 4900 && d[640] < 5100);                           // the echo of the echo, half as loud
    CHECK(d[321] == 0 && d[500] == 0);
    fn_8025D724(rec);
    s32 bb[3][FRAME] = {}; s32* bus[3] = {bb[0], bb[1], bb[2]}; bb[0][0] = 77;
    fn_8025D7D0(bus, rec);
    CHECK(bb[0][0] == 77);                                            // no effect registered: the bus is left alone

    // Chorus: wet output appears after the base delay (10 ms) and stays inside the line.
    memset(rec, 0, sizeof(rec));
    *(f32*)(rec + 0x80) = 10.0f; *(f32*)(rec + 0x84) = 0.3f; *(f32*)(rec + 0x88) = 0.5f; *(f32*)(rec + 0x8C) = 0.0f; *(f32*)(rec + 0x98) = 1.0f;
    fn_8025E3E4(rec);
    std::vector<s32> c = RunImpulse(fn_8025E7F8, rec, 12);
    CHECK(Energy(c, 0, 200) == 0);
    CHECK(Energy(c, 200, 500) > 1e6f);                               // 7 to 13 ms: the modulated tap
    CHECK(Energy(c, 600, c.size()) == 0);                             // no feedback: a single tap
    fn_8025E768(rec);

    // Reverb: a tail that decays, longer with a longer time.
    f32 tail[2];
    for (int k = 0; k < 2; k++) {
        memset(rec, 0, sizeof(rec));
        *(u32*)(rec + 0x110) = 2; *(f32*)(rec + 0x114) = 0.05f; *(f32*)(rec + 0x118) = 0.02f; *(u32*)(rec + 0x11C) = 1;
        *(f32*)(rec + 0x120) = k ? 3.0f : 0.5f; *(f32*)(rec + 0x124) = 0.5f; *(f32*)(rec + 0x128) = 0.3f; *(f32*)(rec + 0x12C) = 0.1f;
        *(f32*)(rec + 0x130) = 0.3f; *(f32*)(rec + 0x134) = 0.5f; *(f32*)(rec + 0x140) = 1.0f;
        AXFXReverbHiExpInit(rec);
        std::vector<s32> v = RunImpulse(fn_8025B780, rec, 700);       // 2.1 s
        CHECK(Energy(v, 0, 600) == 0);                                // nothing before the pre-delay (20 ms)
        CHECK(Energy(v, 640, 6400) > 1e6f);                           // reflections and the first reverb
        tail[k] = Energy(v, 32000, v.size());
        CHECK(Energy(v, 6400, 12800) < Energy(v, 640, 6400) || k == 1);
        AXFXReverbHiExpShutdown(rec);
    }
    CHECK(tail[1] > 10.0f * tail[0]);                                 // 3 s of reverb time rings on, 0.5 s has died
    // Bad parameters (an early set of 9) make no effect.
    memset(rec, 0, sizeof(rec));
    *(u32*)(rec + 0x110) = 9;
    AXFXReverbHiExpInit(rec);
    CHECK(sFx.find(rec) == sFx.end());
    return fails;
}
#endif
