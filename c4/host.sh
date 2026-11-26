#!/bin/bash
# c4/host.sh [OUT]: build Blood on the host with gcc/g++ -m32: Blood-RE's
# game code (source/blood) on chocolate-blood's C engine, audiolib and MACT,
# with c4/blood-host.c for a platform: no display, no sound, virtual time,
# every frame checksummed. The reference the c4 builds are held to.
# Objects and logs in OUT (default ./out/host-$BLOOD_VER); the program is
# OUT/blood-host.
#
# BLOOD_VER is Blood-RE's APPVER_EXEDEF: BS111A (default; shareware 1.11 of
# 23 September 1997), BP121 (Plasma Pak 1.21), or another Blood-RE names.
# qtools has its own (QT110 for 1.10 and 1.11, QT111A from 1.11A on).
#
# g++ uses no C++ library (-nostdinc++; c4/cxxrt.cpp has new and delete),
# because Blood uses none and a 64-bit Linux seldom has the 32-bit one.
R=$(cd "$(dirname "$0")/.." && pwd)
B=$R/source
C=$R/c4
VER=${BLOOD_VER:-BS111A}
case $VER in
	BS110|BSR110|BR110|BP110|BS111|BSR111|BR111|BP111) QT=QT110 ;;
	*) QT=QT111A ;;
esac
O=${1:-$R/out/host-$VER}
mkdir -p $O/obj
INC="-I $C/inc -I $B/blood -I $B/blood/qtools/include -I $B/build -I $B/mact -I $B/audiolib"
COMMON="-m32 -O2 -g -funsigned-char -fno-strict-aliasing -fwrapv -msse2 -mfpmath=sse -ffp-contract=off -fcommon -include $C/inc/c4blood.h -D__C4__ -DC4BLOOD -DMAPEDIT=0 $INC"
CFLAGS="$COMMON -std=gnu99 -fpermissive -w"
CXXFLAGS="$COMMON -std=gnu++17 -nostdinc++ -fno-exceptions -fno-rtti -fpermissive -w"

# chocolate-blood's C: the engine, MMULTI, MACT, audiolib
C_UNITS="build/a_asm.c build/CACHE1D.C build/ENGINE.C build/serialize.c build/video_font.c build/MMULTI.C
mact/ANIMLIB.C mact/control.c mact/file_lib.c mact/keyboard.c mact/mathutil.c mact/scriplib.c
mact/tokenlib.c mact/util_lib.c mact/vidmsdos.c
audiolib/AL_MIDI.C audiolib/BLASTER.C audiolib/DEBUGIO.C audiolib/FX_MAN.C audiolib/GMTIMBRE.C
audiolib/LL_MAN.C audiolib/MIDI.C audiolib/MPU401.C audiolib/MULTIVOC.C audiolib/MUSIC.C
audiolib/mvreverb_asm.c audiolib/mv_mix16_asm.c audiolib/mv_mix_asm.c audiolib/PITCH.C
audiolib/TASK_MAN.C audiolib/USER.C"
# Blood-RE's game (blood/makefile's OBJS, without the mpath, heat and
# engage network drivers), fframe in C
GAME="globals blood screen view db gameutil fire sectorfx map2d satimer usrhooks endgame levels
sound sfx controls actor player replace tile triggers eventq qav weapon gui seq mirrors dude warp
weather credits menu messages ai aibat aibeast aiboneel aiburn aicaleb aicerber aicult aigarg
aigilbst aighost aihand aihound aiinnoc aipod airat aispid aitchern aizomba aizombf fx callback
loadsave config network demo gib asound gamemenu cdrom smkplay choke"
GAME_UNITS="$(for g in $GAME; do printf 'blood/%s.cpp ' $g; done) blood/fframe_asm.c"
# qtools (qtools/makefile's OBJS), crc32 in C, mouse as qmouse; and
# helix32's kernel, in C
QT_UNITS="qheap resource iob trig getopt error debug4g textio pqueue key inifile qmouse misc gfx helix"
QT_UNITS="$(for q in $QT_UNITS; do printf 'blood/qtools/src/%s.cpp ' $q; done) blood/qtools/src/crc32_asm.c"

compile () {
	local u=$1 n o log def
	n=$(echo $u | tr / _ | sed 's/\.[A-Za-z]*$//')
	o=$O/obj/$n.o; log=$O/obj/$n.log
	case $u in
	blood/qtools/*) def=-DAPPVER_EXEDEF=$QT ;;
	blood/*) def=-DAPPVER_EXEDEF=$VER ;;
	*) def= ;;
	esac
	case $u in
	# Blood: Watcom made an enum the smallest type that holds it (GAMEOPTIONS,
	# in demos and saves, is 373 bytes so), and gave a bit-field a unit of its
	# type, as MSVC does (INPUT is 22): -fshort-enums and -mms-bitfields
	*.cpp) g++ $CXXFLAGS -fshort-enums -mms-bitfields $def -c $B/$u -o $o > $log 2>&1 ;;
	blood/*) gcc -x c $CFLAGS -fshort-enums -mms-bitfields $def -c $B/$u -o $o > $log 2>&1 ;;
	# the engine, MACT and audiolib: their own headers first (Blood has a
	# sound.h too)
	*) gcc -x c -I $B/build -I $B/mact -I $B/audiolib $CFLAGS -c $B/$u -o $o > $log 2>&1 ;;
	esac
	if [ $? != 0 ]; then echo "FAIL $u: $(grep -m1 'error' $log | cut -c1-220)"; return 1; fi
}
export -f compile; export B O CFLAGS CXXFLAGS VER QT

: > $O/fails.txt
printf '%s\n' $C_UNITS $GAME_UNITS $QT_UNITS | xargs -P ${JOBS:-4} -I{} bash -c 'compile {}' >> $O/fails.txt
for s in c4blood.c blood-host.c smacker.c; do
	gcc -x c -I $B/build $CFLAGS -c $C/$s -o $O/obj/c4_${s%.c}.o > $O/obj/c4_${s%.c}.log 2>&1 || echo "FAIL c4/$s: $(grep -m1 error $O/obj/c4_${s%.c}.log)" >> $O/fails.txt
done
for s in cxxrt netnone; do
	g++ $CXXFLAGS -c $C/$s.cpp -o $O/obj/c4_$s.o > $O/obj/c4_$s.log 2>&1 || echo "FAIL c4/$s.cpp: $(grep -m1 error $O/obj/c4_$s.log)" >> $O/fails.txt
done
nf=$(wc -l < $O/fails.txt)
echo "host.sh $VER/$QT: $(echo $C_UNITS | wc -w) C units, $(echo $GAME_UNITS $QT_UNITS | wc -w) game units, $nf failed"
sort $O/fails.txt
[ $nf = 0 ] || exit 1
# loadsave first: Blood-RE builds LoadSave::head in a global constructor
# that its 17 savers, each a global, link themselves onto, and link order
# is constructor order (plan section 3.6)
OBJS="$O/obj/blood_loadsave.o $(ls $O/obj/*.o | grep -v /blood_loadsave.o)"
gcc -m32 -g -o $O/blood-host $OBJS -lm > $O/link.log 2>&1 || {
	echo "host.sh: link failed"; grep -E "undefined reference|multiple definition" $O/link.log | sed 's/.*: //' | sort | uniq -c | sort -rn | head -40; exit 1; }
echo "host.sh: $O/blood-host"
