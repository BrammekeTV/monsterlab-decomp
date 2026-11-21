#!/usr/bin/env python3
"""Rewrites the GX pipe writes of an m2c draft with their widths.

m2c prints every store to the write-gather pipe as `*(s8 *)0xCC008000 = X;` and loses the
width. The command stream gives it back: 0x61 is followed by one 32-bit word (BP), 0x08 by a
register byte and a word (CP), 0x10 by two 16-bit words (count - 1, register) and count words (XF).
Anything else (draw commands, a computed opcode) is left as it is and marked `// ?`: read the
widths from `sh tools/dump.sh <name>` (stb / sth / stw / stfs).

    python tools/m2c_fifo.py build/m2c/<name>.c  > build/m2c/<name>.fifo.c
"""
import re
import sys

PIPE = re.compile(r'^(\s*)\*\((?:s8|void) \*\)0xCC008000 = (.*);$')


def num(text):
    try:
        return int(text, 0)
    except ValueError:
        return None


def main():
    lines = open(sys.argv[1]).read().split('\n')
    out = []
    todo = []          # widths still expected for the current command
    for line in lines:
        m = PIPE.match(line)
        if not m:
            if todo and line.strip() and not line.strip().startswith(('sp', 'temp_', 'var_')):
                todo = []          # control flow inside a command: start again
            out.append(line)
            continue
        ind, val = m.groups()
        val = re.sub(r'^\((?:s32|u32|u8|s8|u16|s16)\) ', '', val)
        if not todo:
            op = num(val)
            if op == 0x61:
                todo = ['u32']
            elif op == 0x08:
                todo = ['u8', 'u32']
            elif op == 0x10:
                todo = ['xfcount', 'u16']
            elif op in (0x48, 0x00):
                todo = []
            else:
                out.append('%sGXWGFifo._u8 = %s;   // ? width and command not known' % (ind, val))
                continue
            out.append('%sGXWGFifo._u8 = %s;' % (ind, val))
            continue
        w = todo.pop(0)
        if w == 'xfcount':
            n = num(val)
            todo = ['u16'] + (['word'] * (n + 1) if n is not None else [])
            out.append('%sGXWGFifo._u16 = %s;' % (ind, val))
        elif w == 'word':
            isf = bool(re.search(r'\d\.\d|\df\b|^\(?f32|^temp_f|^var_f|^sp[0-9A-F]+$', val)) and num(val) is None
            out.append('%sGXWGFifo.%s = %s;' % (ind, '_f32' if isf else '_u32', val))
        else:
            out.append('%sGXWGFifo._%s = %s;' % (ind, w, val))
    sys.stdout.write('\n'.join(out))


if __name__ == '__main__':
    main()
