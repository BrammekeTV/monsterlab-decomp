// PC port: the small JSON reader of the mod files (data/strings.json, patches/*.json): errors name the
// line and column. Strings come out as bytes of the game's single-byte text (U+0001-U+00FF).
#ifndef PORT_JSON_H
#define PORT_JSON_H

#include <stdio.h>
#include <string>

#include "types.h"

struct Reader {
    const char* p;
    const char* start;
    std::string err;
    void Fail(const char* what) {
        if (!err.empty()) return;
        int line = 1, col = 1;
        for (const char* c = start; c < p; c++) { if (*c == '\n') { line++; col = 1; } else col++; }
        char buf[220];
        snprintf(buf, sizeof(buf), "line %d, column %d: %s", line, col, what);
        err = buf;
    }
    void Space() { while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++; }
    bool Eat(char c) { Space(); if (*p == c) { p++; return true; } return false; }
    bool Expect(char c) {
        if (Eat(c)) return true;
        char buf[40];
        snprintf(buf, sizeof(buf), "'%c' expected", c);
        Fail(buf);
        return false;
    }
    // A string as bytes of the game's text (single byte: U+0000-U+00FF).
    bool Str(std::string* out) {
        Space();
        if (*p != '"') { Fail("a string in quotes expected"); return false; }
        p++;
        out->clear();
        for (;;) {
            unsigned char c = (unsigned char)*p;
            if (!c) { Fail("the string is not closed"); return false; }
            if (c == '"') { p++; return true; }
            u32 cp;
            if (c == '\\') {
                p++;
                char e = *p++;
                switch (e) {
                case '"': cp = '"'; break;
                case '\\': cp = '\\'; break;
                case '/': cp = '/'; break;
                case 'b': cp = 8; break;
                case 'f': cp = 12; break;
                case 'n': cp = 10; break;
                case 'r': cp = 13; break;
                case 't': cp = 9; break;
                case 'u': {
                    cp = 0;
                    for (int i = 0; i < 4; i++) {
                        char h = *p++;
                        int d = h >= '0' && h <= '9' ? h - '0' : h >= 'a' && h <= 'f' ? h - 'a' + 10 : h >= 'A' && h <= 'F' ? h - 'A' + 10 : -1;
                        if (d < 0) { p--; Fail("four hex digits expected after \\u"); return false; }
                        cp = cp << 4 | (u32)d;
                    }
                    break;
                }
                default: p--; Fail("unknown escape in a string"); return false;
                }
            } else if (c < 0x20) {
                Fail("a line break or control character inside a string (write \\n)");
                return false;
            } else if (c < 0x80) {
                cp = c;
                p++;
            } else {                                         // UTF-8
                int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : -1;
                if (n < 0) { Fail("the file is not UTF-8"); return false; }
                cp = c & (0x3F >> n);
                p++;
                for (int i = 0; i < n; i++) {
                    if ((*p & 0xC0) != 0x80) { Fail("the file is not UTF-8"); return false; }
                    cp = cp << 6 | (u32)(*p++ & 0x3F);
                }
            }
            if (cp > 0xFF) {
                char buf[120];
                snprintf(buf, sizeof(buf), "character U+%04X is not in the game's single-byte text (only U+0000-U+00FF)", cp);
                Fail(buf);
                return false;
            }
            if (cp == 0) { Fail("a string cannot hold U+0000"); return false; }
            out->push_back((char)cp);
        }
    }
};

#endif
