#!/bin/bash
# c4/host.sh [OUT]: build chocolate-blood on the host with gcc/g++ -m32, the
# SDL platform (build/system.c, video.c, sound.c) replaced by
# c4/blood-host.c: no display, no sound, virtual time, every frame
# checksummed. The reference the c4 builds are held to. Objects and logs in
# OUT (default ./out/host); the program is OUT/blood-host.
#
# g++ uses no C++ library (-nostdinc++; c4/cxxrt.cpp has new and delete),
# because Blood uses none and a 64-bit Linux seldom has the 32-bit one.
R=$(cd "$(dirname "$0")/.." && pwd)
B=$R/source
C=$R/c4
O=${1:-$R/out/host}
mkdir -p $O/obj
INC="-I $C/inc -I $B/blood -I $B/blood/qtools/include -I $B/build -I $B/mact -I $B/audiolib"
DEFS="-D__C4__ -DC4BLOOD"
COMMON="-m32 -O2 -g -funsigned-char -fno-strict-aliasing -fwrapv -msse2 -mfpmath=sse -ffp-contract=off -fcommon -include $C/inc/c4blood.h $DEFS $INC"
CFLAGS="$COMMON -std=gnu99 -fpermissive -w"
CXXFLAGS="$COMMON -std=gnu++17 -nostdinc++ -fno-exceptions -fno-rtti -fpermissive -w"

C_UNITS="build/a_asm.c build/CACHE1D.C build/ENGINE.C build/serialize.c build/video_font.c build/MMULTI.C
blood/fframe_asm.c blood/qtools/src/crc32_asm.c
mact/ANIMLIB.C mact/control.c mact/file_lib.c mact/keyboard.c mact/mathutil.c mact/scriplib.c
mact/tokenlib.c mact/util_lib.c mact/vidmsdos.c
audiolib/AL_MIDI.C audiolib/BLASTER.C audiolib/DEBUGIO.C audiolib/FX_MAN.C audiolib/GMTIMBRE.C
audiolib/LL_MAN.C audiolib/MIDI.C audiolib/MPU401.C audiolib/MULTIVOC.C audiolib/MUSIC.C
audiolib/mvreverb_asm.c audiolib/mv_mix16_asm.c audiolib/mv_mix_asm.c audiolib/PITCH.C
audiolib/TASK_MAN.C audiolib/USER.C"
# msvc/blood.vcxproj's C++ units, in its order
CXX_UNITS=$(sed -n 's/.*ClCompile Include="\.\.\\source\\\([^"]*\.cpp\)".*/\1/p' $R/msvc/blood.vcxproj | tr '\\' /)

compile () {
	local u=$1 n o log
	n=$(echo $u | tr / _ | sed 's/\.[A-Za-z]*$//')
	o=$O/obj/$n.o; log=$O/obj/$n.log
	case $u in
	*.cpp) g++ $CXXFLAGS -c $B/$u -o $o > $log 2>&1 ;;
	*) gcc -x c $CFLAGS -c $B/$u -o $o > $log 2>&1 ;;
	esac
	if [ $? != 0 ]; then echo "FAIL $u: $(grep -m1 'error' $log | cut -c1-220)"; return 1; fi
}
export -f compile; export B O CFLAGS CXXFLAGS

: > $O/fails.txt
printf '%s\n' $C_UNITS $CXX_UNITS | xargs -P ${JOBS:-4} -I{} bash -c 'compile {}' >> $O/fails.txt
for s in c4blood.c blood-host.c; do
	gcc -x c $CFLAGS -c $C/$s -o $O/obj/c4_${s%.c}.o > $O/obj/c4_${s%.c}.log 2>&1 || echo "FAIL c4/$s: $(grep -m1 error $O/obj/c4_${s%.c}.log)" >> $O/fails.txt
done
g++ $CXXFLAGS -c $C/cxxrt.cpp -o $O/obj/c4_cxxrt.o > $O/obj/c4_cxxrt.log 2>&1 || echo "FAIL c4/cxxrt.cpp: $(grep -m1 error $O/obj/c4_cxxrt.log)" >> $O/fails.txt
nf=$(wc -l < $O/fails.txt)
echo "host.sh: $(echo $C_UNITS | wc -w) C units, $(echo $CXX_UNITS | wc -w) C++ units, $nf failed"
sort $O/fails.txt
[ $nf = 0 ] || exit 1
gcc -m32 -g -o $O/blood-host $O/obj/*.o -lm > $O/link.log 2>&1 || {
	echo "host.sh: link failed"; grep -E "undefined reference|multiple definition" $O/link.log | sed 's/.*: //' | sort | uniq -c | sort -rn | head -40; exit 1; }
echo "host.sh: $O/blood-host"
