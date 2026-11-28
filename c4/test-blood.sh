#!/bin/bash
# c4/test-blood.sh [FRAMES] -- c4's test-blood (c4.mk), from c4's root: the
# Blood on C4IX's disk, c4's display build (blood-c4.c), plays the
# shareware's demo loop inside C4IX as c4-Blood's host build (B0) does,
# every frame's checksum B0's, for FRAMES frames (default 300). A test: on
# blood-host.c's virtual clock (-sum 1), with no sound, as B0 plays it.
#
# B0 is built by c4's src/c4cxx/blood-b0.sh (globals in the source's order,
# Watcom's rand), once, into W/ref. Both play from one directory of the
# shareware's files, with c4's BLOOD.CFG but no sound card (devices 13, as
# B0 has none). Blood finds its demo loop with _dos_findfirst (c4blood.c:
# opendir) and plays the demos in the directory's order, so C4IX's
# directory must list them as the host's does: the test's disk is the
# image's, as links, with /usr/games/blood made again in that order (C4IX
# lists a directory newest entry first: the last in c4ix.vfs.txt), as
# src/c4cxx/blood-c4lc.sh does for the headless build.
#
# BLOOD_DIR is c4-Blood (default games/blood), BLOOD_RUN the shareware's
# files (default its data/blood111), C4_IMAGES the images (default
# src/c4bb/images), W the work directory (default .blood/test), CLI_FLAGS
# cli.js's own (--no-accel: without the accelerators).
R=$(pwd)
FRAMES=${1:-300}
G=${BLOOD_DIR:-$R/games/blood}
RUN=${BLOOD_RUN:-$G/data/blood111}
IMG=${C4_IMAGES:-$R/src/c4bb/images}
W=${W:-$R/.blood/test}
[ -f $IMG/disk/c4ix-blood.c4r ] || { echo "test-blood: no c4ix-blood.c4r in $IMG/disk (make c4bb-images, with the data)"; exit 1; }
[ -f $RUN/BLOOD.RFF ] || { echo "test-blood: no shareware in $RUN (make blood-data)"; exit 1; }
mkdir -p $W
# 1. B0, once
if [ ! -x $W/ref/blood-host ] || [ ! -f $W/ref/rand.o ]; then
	BLOOD_DIR=$G bash src/c4cxx/blood-b0.sh $W/ref > $W/b0.log 2>&1 || { echo "test-blood: B0 did not build"; tail -5 $W/b0.log; exit 1; }
fi
# 2. the directory: the shareware's files, and c4's BLOOD.CFG with no card
rm -rf $W/run && mkdir -p $W/run
for f in $RUN/*; do ln -s $(readlink -f $f) $W/run/; done
sed 's/^FXDevice = .*/FXDevice = 13/; s/^MusicDevice = .*/MusicDevice = 13/' $G/c4/BLOOD.CFG > $W/run/BLOOD.CFG
# ... and the game itself, as /usr/games/blood has it: Blood takes every
# file in its directory as a resource it may be asked for, and how many
# there are changes its heap (docs/c4cxx-plan.md 4.11)
cp $IMG/disk/c4ix-blood.c4r $W/run/blood.c4r
# ... and what C4IX has there besides: the repository's own files and
# directories (/usr/games is the disk's games/, its sources for the IDE)
SRC=$(ls -A $IMG/disk/games/blood 2>/dev/null | grep -v '^\.c4ix-dir$')
for f in $SRC; do ln -s $(readlink -f $IMG/disk/games/blood/$f) $W/run/$f; done
NAMES=$(cd $W/run && ls -f | grep -v '^\.\.\?$' | grep -vx blood.c4r | grep -vxF "$(echo $SRC | tr ' ' '
')")
sums () { tr -d '\r' < $1 | grep "blood-host: frame" | sed 's/ t=.*//; s/^.*blood-host: /blood-host: /'; }
(cd $W/run && timeout 3000 $W/ref/blood-host -sum 1 -frames $FRAMES) > $W/b0.out 2>&1
sums $W/b0.out > $W/b0.sums
n=$(wc -l < $W/b0.sums)
[ "$n" = "$FRAMES" ] || { echo "test-blood: B0 played $n frames of $FRAMES"; tail -3 $W/b0.out; exit 1; }
# 3. C4IX's disk, as links, with the test's /usr/games/blood
rm -rf $W/disk && mkdir -p $W/disk
for f in $(ls -A $IMG/disk); do ln -s $IMG/disk/$f $W/disk/$f; done
rm $W/disk/c4ix.vfs.txt $W/disk/c4ix.sizes
# the game's files are the test's, not the image's (whose BLOOD.CFG has the
# sound card on)
rm -f $W/disk/blood-*
for f in $NAMES; do ln -s $(readlink -f $W/run/$f) $W/disk/blood-$f; done
# the image's block for blood/ out, the test's in its place
awk -v names="$NAMES" '
	BEGIN { k = split(names, nm, "\n") }
	skip && index($0, pad "\t") == 1 { next }
	{ skip = 0 }
	/^\t*blood\/:$/ {
		pad = substr($0, 1, index($0, "blood/:") - 1)
		print pad "blood/:"; print pad "\tblood.c4r: c4ix-blood.c4r"
		for (i = k; i >= 1; i--) print pad "\t" nm[i] ": blood-" nm[i]
		skip = 1; done = 1; next
	}
	{ print }
	END { if (!done) exit 1 }' $IMG/disk/c4ix.vfs.txt > $W/disk/c4ix.vfs.txt \
	|| { echo "test-blood: no /usr/games/blood in $IMG/disk/c4ix.vfs.txt"; exit 1; }
{
	for f in $NAMES; do echo "$(stat -L -c %s $W/disk/blood-$f) blood-$f"; done
	grep -v ' blood-' $IMG/disk/c4ix.sizes
} > $W/disk/c4ix.sizes
# 4. the demo loop inside C4IX
printf 'cd /usr/games/blood\n./blood.c4r -sum 1 -frames %s\nexit\n' $FRAMES | timeout ${C4_TIMEOUT:-6000} \
	node libjs/cli.js ${CLI_FLAGS:-} -m ${C4_MEM:-256} -d $W/disk $IMG/c4ix32.c4r > $W/c4ix.out 2>&1
sums $W/c4ix.out > $W/c4ix.sums
if ! cmp -s $W/b0.sums $W/c4ix.sums; then
	echo "test-blood: C4IX: $(wc -l < $W/c4ix.sums) frames; they differ from $(diff $W/b0.sums $W/c4ix.sums | grep -m1 '^<' | sed 's/^< blood-host: //')"
	tail -5 $W/c4ix.out
	exit 1
fi
echo "test-blood: C4IX, the display build: $FRAMES frames, every checksum B0's ($(tail -1 $W/c4ix.sums | sed 's/.*total/total/'))"
echo "test-blood: OK"
