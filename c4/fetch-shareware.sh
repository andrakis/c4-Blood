#!/bin/bash
# c4/fetch-shareware.sh [DIR]: Blood's shareware episode, version 1.11
# (Monolith, 1997-09), into DIR (default ./data/blood111). Fetched from
# classicdosgames.com, or taken from $BLOOD_EXE, checked by its SHA-256, and
# unpacked with c4/tools/unshr.py (the installer's SHARE.SHR is PKWARE DCL).
# Never committed. Its terms (BLOODHLP.TXT): "You may freely distribute Blood
# shareware provided that you include all the original files and that you do
# not charge for distribution."
set -e
R=$(cd "$(dirname "$0")/.." && pwd)
D=${1:-$R/data/blood111}
URL=https://classicdosgames.com/files/games/monolith/blood.exe
SHA=b0b07c03dd86b9776dc98b3df7d08d97b904bd8763ce3b34dc22a4219314a485
SIZE=17349441
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
if [ -n "$BLOOD_EXE" ]; then cp "$BLOOD_EXE" $T/blood.exe; else curl -sSfL -o $T/blood.exe $URL; fi
[ "$(stat -c %s $T/blood.exe)" = $SIZE ] || { echo "fetch-shareware: blood.exe is not $SIZE bytes"; exit 1; }
echo "$SHA  $T/blood.exe" | sha256sum -c --quiet || { echo "fetch-shareware: blood.exe has the wrong SHA-256"; exit 1; }
# the installer is a self-extracting zip holding SHARE.SHR
python3 -I -c 'import sys, zipfile; zipfile.ZipFile(sys.argv[1]).extract("SHARE.SHR", sys.argv[2])' $T/blood.exe $T
python3 -I $R/c4/tools/unshr.py $T/SHARE.SHR "$D"
echo "fetch-shareware: $D"
