#!/bin/bash
# c4/c4-objs.sh -- Blood's objects for c4's images (c4.mk's blood-objs), from
# c4's root: every C unit through c4lc, every C++ unit through c4cxx and
# then c4lc, into .blood, as c4cxx's blood-c4lc.sh builds the headless game
# (c4's docs/c4cxx-plan.md 4.11) -- but with c4/blood-c4.c's platform, a
# display and a sound card, in place of blood-host.c's, and blood-snd.c's
# effects in place of audiolib's FX_MAN.C and its mixer. A unit is built
# again when it, or any header, is newer than its object.
#
#   .blood/*.c4o        the game, linked first by c4-images.sh: loadsave.c4o
#                       first of all (its constructor builds the list the
#                       savers join)
#   .blood/plat/*.c4o   blood-c4ix.c and blood-bare.c, one for each system
#
# LC is the c4lc command (lc32, with c4's image flags), JOBS how many at once.
# Each unit is a wrapper in .blood/c that includes c4/inc/c4blood.h and then
# the unit, by its path from c4's root (-I .), as host.sh's -include does.
G=${BLOOD:-games/blood}
B=$G/source C=$G/c4
VER=${BLOOD_VER:-BS111A}
case $VER in
	BS110|BSR110|BR110|BP110|BS111|BSR111|BR111|BP111) QT=QT110 ;;
	*) QT=QT111A ;;
esac
O=.blood
mkdir -p $O/plat $O/c
LC="${LC:?LC: the c4lc command} -O -fno-builtin -c -conforming -funsigned-char"
WF="-fshort-enums -mms-bitfields"
DEF="-D __C4__ -D C4BLOOD -D C4BLOOD_C4 -D MAPEDIT=0"
GINC="-I $B/blood -I $B/blood/qtools/include -I $B/build -I $B/mact -I $B/audiolib"
LIBC="-I src/c4ix/libc/include -I src/c4ix/include -I libjs/guest"
# chocolate-blood's engine, MACT and audiolib (but FX_MAN.C and MultiVoc,
# whose part blood-snd.c plays), fframe and crc32 in C
C_UNITS="build/a_asm.c build/CACHE1D.C build/ENGINE.C build/serialize.c build/video_font.c build/MMULTI.C
mact/ANIMLIB.C mact/control.c mact/file_lib.c mact/keyboard.c mact/mathutil.c mact/scriplib.c
mact/tokenlib.c mact/util_lib.c mact/vidmsdos.c
audiolib/AL_MIDI.C audiolib/BLASTER.C audiolib/DEBUGIO.C audiolib/GMTIMBRE.C
audiolib/LL_MAN.C audiolib/MIDI.C audiolib/MPU401.C audiolib/MUSIC.C
audiolib/TASK_MAN.C audiolib/USER.C blood/fframe_asm.c blood/qtools/src/crc32_asm.c"
GAME="globals blood screen view db gameutil fire sectorfx map2d satimer usrhooks endgame levels
sound sfx controls actor player replace tile triggers eventq qav weapon gui seq mirrors dude warp
weather credits menu messages ai aibat aibeast aiboneel aiburn aicaleb aicerber aicult aigarg
aigilbst aighost aihand aihound aiinnoc aipod airat aispid aitchern aizomba aizombf fx callback
loadsave config network demo gib asound gamemenu cdrom smkplay choke"
QTU="qheap resource iob trig getopt error debug4g textio pqueue key inifile qmouse misc gfx helix"
CXX_UNITS="$(for g in $GAME; do echo blood/$g; done) $(for q in $QTU; do echo blood/qtools/src/$q; done) c4/netnone"
C4_UNITS="c4blood smacker blood-c4 blood-gui blood-snd"
PLAT_UNITS="blood-c4ix blood-bare"

# the newest header: a unit older than it is built again (blood-host.c
# counts as one: blood-c4.c includes it)
HDRS=$(ls -t $B/blood/*.h $B/blood/qtools/include/*.h $B/build/*.h $B/mact/*.H $B/audiolib/*.H \
	$C/inc/*.h $C/inc/SDL3/*.h $C/c4inc/*.h $C/*.h $C/blood-host.c src/c4ix/libc/include/*.h libjs/guest/gui.[ch] \
	libjs/guest/snd.[ch] src/c4ix/include/*.h 2>/dev/null | head -1)
stale () { [ ! -s $1 ] || [ $2 -nt $1 ] || [ $HDRS -nt $1 ] || [ ./c4cxx -nt $1 ] || [ src/c4sp/lisp/c4lc.lisp -nt $1 ]; }

# one job: KIND OBJECT SOURCE, the flags in the environment's F
unit () {
	local kind=$1 o=$2 src=$3 w n
	n=$(basename $o .c4o)
	if [ $kind = cxx ]; then
		printf '#include "c4blood.h"\n#include "%s"\n' $src > $O/c/w_$n.cpp
		./c4cxx -x c++ $WF -funsigned-char -fargs-rtl -I . $F -isystem src/c4ix/libc/include -isystem src/c4ix/include \
			-o $O/c/$n.c $O/c/w_$n.cpp > $O/c/$n.err 2>&1 \
			|| { echo "blood-objs: c4cxx $src: $(grep -m1 error $O/c/$n.err | cut -c1-200)"; return 1; }
		$LC -P $WF $O/c/$n.c $o > $O/c/$n.lc 2>&1 && [ -s $o ] \
			|| { echo "blood-objs: c4lc $src: $(grep -v '^;;' $O/c/$n.lc | grep -v '^warning' | head -1 | cut -c1-200)"; rm -f $o; return 1; }
	else
		printf '#include "c4blood.h"\n#include "%s"\n' $src > $O/c/w_$n.c
		$LC -I . $F $O/c/w_$n.c $o > $O/c/$n.lc 2>&1 && [ -s $o ] \
			|| { echo "blood-objs: c4lc $src: $(grep -v '^;;' $O/c/$n.lc | grep -v '^warning' | head -1 | cut -c1-200)"; rm -f $o; return 1; }
	fi
}
export -f unit; export LC WF O

jobs=$O/jobs.txt
: > $jobs
add () { echo "$1|$2|$3|$4" >> $jobs; }
for u in $C_UNITS; do
	n=$(echo $u | tr / _ | sed 's/\.[A-Za-z]*$//')
	case $u in
	blood/qtools/*) f="$WF -D APPVER_EXEDEF=$QT $DEF -I $C/c4inc -I $C/inc $GINC $LIBC" ;;
	blood/*) f="$WF -D APPVER_EXEDEF=$VER $DEF -I $C/c4inc -I $C/inc $GINC $LIBC" ;;
	# the engine, MACT and audiolib: their own headers first (Blood has a
	# sound.h too)
	*) f="$DEF -I $B/build -I $B/mact -I $B/audiolib -I $C/c4inc -I $C/inc $GINC $LIBC" ;;
	esac
	stale $O/$n.c4o $B/$u && add c $O/$n.c4o $B/$u "$f"
done
for s in $C4_UNITS; do
	# the platform: the engine's and audiolib's headers, but not the
	# game's (its gui.h and sound.h are not libjs's and the engine's)
	stale $O/c4_$s.c4o $C/$s.c && add c $O/c4_$s.c4o $C/$s.c "$DEF -I $C -I $B/build -I $B/audiolib -I $C/c4inc -I $C/inc $LIBC"
done
for s in $PLAT_UNITS; do
	stale $O/plat/$s.c4o $C/$s.c && add c $O/plat/$s.c4o $C/$s.c "$DEF -I $C -I $C/c4inc -I $C/inc $LIBC"
done
for u in $CXX_UNITS; do
	case $u in blood/qtools/*) def="-D APPVER_EXEDEF=$QT" ;; blood/*) def="-D APPVER_EXEDEF=$VER" ;; *) def= ;; esac
	src=$B/$u.cpp
	[ -f $src ] || src=$G/$u.cpp
	n=$(echo $u | tr / _)
	stale $O/$n.c4o $src && add cxx $O/$n.c4o $src "$DEF $def -I $C/c4inc -I $C/inc $GINC"
done
# c4cxx's runtime (new, delete, the pure-virtual trap), after c4blood.h too
stale $O/libc4cxx.c4o src/c4cxx/rt/libc4cxx.c && add c $O/libc4cxx.c4o src/c4cxx/rt/libc4cxx.c "$DEF -I $C/c4inc -I $C/inc $LIBC"

n=$(wc -l < $jobs)
[ $n = 0 ] && exit 0
echo "blood-objs: $n units through c4lc"
fails=$O/fails.txt
: > $fails
tr '\n' '\0' < $jobs | xargs -0 -P ${JOBS:-4} -I{} bash -c 'IFS="|" read k o s F <<< "{}"; F="$F" unit $k $o $s' >> $fails 2>&1
if [ -s $fails ]; then cat $fails; exit 1; fi
exit 0
