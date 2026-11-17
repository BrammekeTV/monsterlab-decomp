#!/bin/sh
# Print a function's asm compactly: tools/fn.sh NAME
f=$(grep -l "^\.fn $1," build/RM7P4F/asm/*.s </dev/null | head -1)
[ -z "$f" ] && { echo "not found: $1"; exit 1; }
awk -v n="$1" '$0 ~ "^\.fn "n"," {p=1} p {print} $0 ~ "^\.endfn "n"$" {exit}' "$f" | sed 's#^/\* \([0-9A-F]*\) [0-9A-F]*  [0-9A-F ]* \*/#\1#'
