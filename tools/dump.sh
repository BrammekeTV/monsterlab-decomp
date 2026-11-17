#!/bin/sh
# Compact asm dump of one or more functions: tools/dump.sh name...
for f in "$@"; do
  file=$(grep -l "^\.fn $f," build/RM7P4F/asm/*.s | head -1)
  awk -v f="$f" '$0 ~ "^[.]fn "f"," {p=1} p {print} p && $0 ~ "^[.]endfn "f"$" {exit}' "$file" \
   | sed -E 's|^/\* ([0-9A-F]{8}) [0-9A-F]{8}  [0-9A-F ]{11} \*/\t|\1 |' | grep -v '^\s*$'
done
