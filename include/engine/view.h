#pragma once
#include "types.h"

struct View;   // src/engine/view.cpp

extern "C" {
View* ViewCreate(void);         // 0x80170374
void  ViewDestroy(View* view);  // 0x801704D8
}
