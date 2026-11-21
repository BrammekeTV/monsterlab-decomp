// Missing functions batch 583: vector constant writers. Each one stores a constant
// (x, y, z) = (a, b, a) into a 12-byte global Vec3 at 0x803E6Axx / 0x803E6Bxx.
#include "types.h"
#include "engine/math.h"

extern "C" {
extern Vec3 lbl_803E6A28;                 // 0x803E6A28
extern Vec3 lbl_803E6A38;                 // 0x803E6A38
extern Vec3 lbl_803E6A58;                 // 0x803E6A58
extern Vec3 lbl_803E6A68;                 // 0x803E6A68
extern Vec3 lbl_803E6A78;                 // 0x803E6A78
extern Vec3 lbl_803E6A88;                 // 0x803E6A88
extern Vec3 lbl_803E6A98;                 // 0x803E6A98
extern Vec3 lbl_803E6AA8;                 // 0x803E6AA8
extern Vec3 lbl_803E6AB8;                 // 0x803E6AB8
extern Vec3 lbl_803E6AC8;                 // 0x803E6AC8
extern Vec3 lbl_803E6AD8;                 // 0x803E6AD8
extern Vec3 lbl_803E6AE8;                 // 0x803E6AE8
extern Vec3 lbl_803E6AF8;                 // 0x803E6AF8
extern Vec3 lbl_803E6B08;                 // 0x803E6B08
extern Vec3 lbl_803E6B18;                 // 0x803E6B18
extern Vec3 lbl_803E6B40;                 // 0x803E6B40
extern Vec3 lbl_803E6B50;                 // 0x803E6B50
extern Vec3 lbl_803E6B60;                 // 0x803E6B60
extern Vec3 lbl_803E6B70;                 // 0x803E6B70
extern Vec3 lbl_803E6B80;                 // 0x803E6B80

extern f32 lbl_805FEC00;                  // 0x805FEC00 (float constant)
extern f32 lbl_805FEC04;                  // 0x805FEC04 (float constant)
extern f32 lbl_805FEC08;                  // 0x805FEC08 (float constant)
extern f32 lbl_805FEC0C;                  // 0x805FEC0C (float constant)
extern f32 lbl_805FEC10;                  // 0x805FEC10 (float constant)
extern f32 lbl_805FEC14;                  // 0x805FEC14 (float constant)
extern f32 lbl_805FEC18;                  // 0x805FEC18 (float constant)
extern f32 lbl_805FEC1C;                  // 0x805FEC1C (float constant)
extern f32 lbl_805FEC50;                  // 0x805FEC50 (float constant)
extern f32 lbl_805FEC68;                  // 0x805FEC68 (float constant)
extern f32 lbl_805FEC70;                  // 0x805FEC70 (float constant)
extern f32 lbl_805FEC90;                  // 0x805FEC90 (float constant)
extern f32 lbl_805FEC98;                  // 0x805FEC98 (float constant)
extern f32 lbl_805FEC9C;                  // 0x805FEC9C (float constant)
extern f32 lbl_805FECA0;                  // 0x805FECA0 (float constant)
extern f32 lbl_805FECA4;                  // 0x805FECA4 (float constant)
extern f32 lbl_805FECA8;                  // 0x805FECA8 (float constant)
extern f32 lbl_805FECAC;                  // 0x805FECAC (float constant)
extern f32 lbl_805FECB0;                  // 0x805FECB0 (float constant)
extern f32 lbl_805FECB4;                  // 0x805FECB4 (float constant)
extern f32 lbl_805FECBC;                  // 0x805FECBC (float constant)
extern f32 lbl_805FECC0;                  // 0x805FECC0 (float constant)
extern f32 lbl_805FECC8;                  // 0x805FECC8 (float constant)
extern f32 lbl_805FECCC;                  // 0x805FECCC (float constant)
extern f32 lbl_805FECD0;                  // 0x805FECD0 (float constant)
extern f32 lbl_805FECD4;                  // 0x805FECD4 (float constant)
extern f32 lbl_805FECE0;                  // 0x805FECE0 (float constant)
extern f32 lbl_805FECE4;                  // 0x805FECE4 (float constant)
extern f32 lbl_805FECE8;                  // 0x805FECE8 (float constant)
extern f32 lbl_805FECEC;                  // 0x805FECEC (float constant)
extern f32 lbl_805FECF0;                  // 0x805FECF0 (float constant)
extern f32 lbl_805FECF4;                  // 0x805FECF4 (float constant)
extern f32 lbl_805FECF8;                  // 0x805FECF8 (float constant)
extern f32 lbl_805FECFC;                  // 0x805FECFC (float constant)
extern f32 lbl_805FED00;                  // 0x805FED00 (float constant)
extern f32 lbl_805FED04;                  // 0x805FED04 (float constant)
extern f32 lbl_805FED08;                  // 0x805FED08 (float constant)
extern f32 lbl_805FED0C;                  // 0x805FED0C (float constant)
extern f32 lbl_805FED10;                  // 0x805FED10 (float constant)
extern f32 lbl_805FED14;                  // 0x805FED14 (float constant)
}

extern "C" {

// 0x8010A614
void fn_8010A614(void) {
    lbl_803E6A28.x = lbl_805FEC00;
    lbl_803E6A28.y = lbl_805FEC04;
    lbl_803E6A28.z = lbl_805FEC00;
}

// 0x8010A658
void fn_8010A658(void) {
    lbl_803E6A38.x = lbl_805FEC08;
    lbl_803E6A38.y = lbl_805FEC0C;
    lbl_803E6A38.z = lbl_805FEC08;
}

// 0x8010A74C
void fn_8010A74C(void) {
    lbl_803E6A58.x = lbl_805FEC10;
    lbl_803E6A58.y = lbl_805FEC14;
    lbl_803E6A58.z = lbl_805FEC10;
}

// 0x8010B14C
void fn_8010B14C(void) {
    lbl_803E6A68.x = lbl_805FEC18;
    lbl_803E6A68.y = lbl_805FEC1C;
    lbl_803E6A68.z = lbl_805FEC18;
}

// 0x8010B48C
void fn_8010B48C(void) {
    lbl_803E6A78.x = lbl_805FEC50;
    lbl_803E6A78.y = lbl_805FEC68;
    lbl_803E6A78.z = lbl_805FEC50;
}

// 0x8010C730
void fn_8010C730(void) {
    lbl_803E6A88.x = lbl_805FEC70;
    lbl_803E6A88.y = lbl_805FEC90;
    lbl_803E6A88.z = lbl_805FEC70;
}

// 0x8010CCF8
void fn_8010CCF8(void) {
    lbl_803E6A98.x = lbl_805FEC9C;
    lbl_803E6A98.y = lbl_805FEC98;
    lbl_803E6A98.z = lbl_805FEC9C;
}

// 0x8010FA28
void fn_8010FA28(void) {
    lbl_803E6AA8.x = lbl_805FECA0;
    lbl_803E6AA8.y = lbl_805FECA4;
    lbl_803E6AA8.z = lbl_805FECA0;
}

// 0x8010FCF4
void fn_8010FCF4(void) {
    lbl_803E6AB8.x = lbl_805FECA8;
    lbl_803E6AB8.y = lbl_805FECAC;
    lbl_803E6AB8.z = lbl_805FECA8;
}

// 0x8010FD84
void fn_8010FD84(void) {
    lbl_803E6AC8.x = lbl_805FECB0;
    lbl_803E6AC8.y = lbl_805FECB4;
    lbl_803E6AC8.z = lbl_805FECB0;
}

// 0x80116BC8
void fn_80116BC8(void) {
    lbl_803E6AD8.x = lbl_805FECBC;
    lbl_803E6AD8.y = lbl_805FECC0;
    lbl_803E6AD8.z = lbl_805FECBC;
}

// 0x80116C88
void fn_80116C88(void) {
    lbl_803E6AE8.x = lbl_805FECC8;
    lbl_803E6AE8.y = lbl_805FECCC;
    lbl_803E6AE8.z = lbl_805FECC8;
}

// 0x80117070
void fn_80117070(void) {
    lbl_803E6AF8.x = lbl_805FECD0;
    lbl_803E6AF8.y = lbl_805FECD4;
    lbl_803E6AF8.z = lbl_805FECD0;
}

// 0x80117AC4
void fn_80117AC4(void) {
    lbl_803E6B08.x = lbl_805FECE0;
    lbl_803E6B08.y = lbl_805FECE4;
    lbl_803E6B08.z = lbl_805FECE0;
}

// 0x801196D4
void fn_801196D4(void) {
    lbl_803E6B18.x = lbl_805FECE8;
    lbl_803E6B18.y = lbl_805FECEC;
    lbl_803E6B18.z = lbl_805FECE8;
}

// 0x8011A39C
void fn_8011A39C(void) {
    lbl_803E6B40.x = lbl_805FECF0;
    lbl_803E6B40.y = lbl_805FECF4;
    lbl_803E6B40.z = lbl_805FECF0;
}

// 0x8011A758
void fn_8011A758(void) {
    lbl_803E6B50.x = lbl_805FECF8;
    lbl_803E6B50.y = lbl_805FECFC;
    lbl_803E6B50.z = lbl_805FECF8;
}

// 0x8011C440
void fn_8011C440(void) {
    lbl_803E6B60.x = lbl_805FED00;
    lbl_803E6B60.y = lbl_805FED04;
    lbl_803E6B60.z = lbl_805FED00;
}

// 0x8011D1A0
void fn_8011D1A0(void) {
    lbl_803E6B70.x = lbl_805FED08;
    lbl_803E6B70.y = lbl_805FED0C;
    lbl_803E6B70.z = lbl_805FED08;
}

// 0x8011D5A4
void fn_8011D5A4(void) {
    lbl_803E6B80.x = lbl_805FED10;
    lbl_803E6B80.y = lbl_805FED14;
    lbl_803E6B80.z = lbl_805FED10;
}

}  // extern "C"
