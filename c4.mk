# c4.mk -- this game's part of c4's Makefile. c4 includes it from
# games/blood/c4.mk, its submodule, and runs it from c4's root, with c4's
# tools and variables (games/README.md in c4). Without it, c4 builds
# everything else, and test-blood says it was skipped.

# Blood (games/blood/README.c4.md): Blood-RE's game on chocolate-blood's
# engine, MACT and audiolib. Its C++ goes through c4cxx, c4's translator
# to C, and every unit through c4lc (c4/c4-objs.sh), with c4/blood-c4.c's
# platform: a display and a sound card. c4-images.sh links them for C4IX,
# with the shareware episode beside it.
BLOOD      := games/blood
BLOOD_SRCS := $(wildcard $(BLOOD)/source/blood/*.cpp $(BLOOD)/source/blood/*.c $(BLOOD)/source/blood/*.h \
              $(BLOOD)/source/blood/qtools/src/* $(BLOOD)/source/blood/qtools/include/* \
              $(BLOOD)/source/build/* $(BLOOD)/source/mact/* $(BLOOD)/source/audiolib/* \
              $(BLOOD)/c4/*.c $(BLOOD)/c4/*.cpp $(BLOOD)/c4/*.h $(BLOOD)/c4/inc/*.h $(BLOOD)/c4/inc/SDL3/*.h \
              $(BLOOD)/c4/c4inc/*.h) \
              $(wildcard src/c4ix/libc/include/*.h src/c4ix/include/*.h) \
              libjs/guest/gui.h libjs/guest/gui.c libjs/guest/snd.h libjs/guest/snd.c src/c4cxx/rt/libc4cxx.c
blood-objs: .blood/c4_blood-c4.c4o
.blood/c4_blood-c4.c4o: $(BLOOD_SRCS) $(BLOOD)/c4/c4-objs.sh c4cxx $(C4LC_LISP) .img_g | $(LC32_BIN)
	@LC="$(call lc32,-c 400000) $(IMG_G)" bash $(BLOOD)/c4/c4-objs.sh
PHONY += blood-objs

# The data, which is not in the repository: Monolith's shareware episode,
# version 1.11, as it was distributed (c4/fetch-shareware.sh: checked by
# its SHA-256, unpacked to data/blood111). Its terms (BLOODHLP.TXT) let it
# be passed on with all its original files, free of charge: so all of them
# go on the disk. fetch-shareware.sh takes the installer from $BLOOD_EXE
# when that names a copy, or else from the web.
BLOOD_DATA := $(BLOOD)/data/blood111
blood-data: $(BLOOD_DATA)/BLOOD.RFF
$(BLOOD_DATA)/BLOOD.RFF:
	@bash $(BLOOD)/c4/fetch-shareware.sh $(abspath $(BLOOD_DATA))
PHONY += blood-data

# Blood's accelerators (c4/accel.mjs, and c4pc's src/c4pc/games/blood.c)
# against a_asm.c's own loops, which c4/acceltest.c includes: on the bare
# machine, in a directory of its own (c4-images.sh links .blood/*.c4o)
BLOOD_TEST_CC := $(call lc32,-c 400000) -O $(IMG_G) -fno-builtin -c -conforming -funsigned-char -D __C4__ -D C4BLOOD -D C4BLOOD_C4 \
                 -I $(BLOOD)/c4/inc -I $(BLOOD)/c4/c4inc -I $(BLOOD)/source/build -I src/c4ix/libc/include -I libjs/guest
.blood/test/acceltest.c4o: $(BLOOD)/c4/acceltest.c $(BLOOD)/source/build/a_asm.c $(BLOOD)/c4/inc/c4accel.h \
                           $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .blood/test
	$(BLOOD_TEST_CC) $< $@ > /dev/null
.blood/test/acceltest.c4r: .blood/test/acceltest.c4o libc-bare-objs
	./c4rlink32 $< .libc-bare/libc-*.c4o -o $@ > /dev/null

# test-blood: c4's build of Blood, with its display and sound: first its
# accelerators against their loops, on libjs and on c4pc-host where this
# tree has one; then the shareware's demo loop played inside C4IX (with
# libjs's accelerators), every frame's checksum by the host build's (B0),
# on its virtual clock (-sum 1). c4cxx's blood-c4lc.sh holds the headless
# build to B0 too, and test-c4pc starts this one from the desktop.
BLOOD_FRAMES ?= 300
test-blood: blood-data c4bb-images .blood/test/acceltest.c4r
	@BLOOD_DIR=$(abspath $(BLOOD)) BLOOD_RUN=$(abspath $(BLOOD_DATA)) bash $(BLOOD)/c4/test-blood.sh $(BLOOD_FRAMES)
PHONY += test-blood

GAME_OBJS  += blood-objs
GAME_TESTS += test-blood
