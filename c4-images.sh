# c4-images.sh c4ix|bare -- Blood's part of c4's build-images.sh, which
# sources it from c4's root (games/README.md in c4)
GAMEDIR=games/blood
# the shareware's files, all of them, as its terms want (BLOODHLP.TXT):
# c4/fetch-shareware.sh's data/blood111
BLOOD_FILES="BLOOD.EXE BLOOD.INI BLOOD.RFF BLOOD000.DEM BLOOD001.DEM BLOOD002.DEM BLOODHLP.TXT COMMIT.DAT COMMIT.EXE DOS4GW.EXE GUI.RFF MODEM.PCK MONOLITH.TXT README.TXT SETMAIN.EXE SETUP.EXE SHARE000.ART SOUNDS.RFF SURFACE.DAT TABLES.DAT TENGAME.INI UNSTALL.EXE VOXEL.DAT"
case "$1" in
c4ix)
    # linked against C4IX's libc, loadsave first (its constructor builds
    # the list the savers join), with the shareware beside it (c4ix.vfs.txt:
    # /usr/games/blood). C4IX's disk is flat and shared, so each of the
    # game's files goes there as blood-NAME. Without the data, no game.
    if [ -f .blood/c4_blood-c4.c4o ] && [ -f $PB/lib/libc.c4s ] && [ -f $GAMEDIR/data/blood111/BLOOD.RFF ]; then
        say "linking Blood for C4IX"
        ./c4rlink32 .blood/blood_loadsave.c4o $(ls .blood/*.c4o | grep -v '/blood_loadsave.c4o$')             .blood/plat/blood-c4ix.c4o -l $PB/lib/libc.c4s -o $DISK/c4ix-blood.c4r > /dev/null
        for f in $BLOOD_FILES; do cp $GAMEDIR/data/blood111/$f $DISK/blood-$f; done
        # c4's setup: the effects on the card (FXDevice 0) and the music on
        # its OPL, as an AdLib (MusicDevice 3)
        cp $GAMEDIR/c4/BLOOD.CFG $DISK/blood-BLOOD.CFG
        # the Build engine's licence travels with the engine
        cp $GAMEDIR/BUILDLIC.TXT $DISK/blood-buildlic.txt
    else
        rm -f $DISK/c4ix-blood.c4r
    fi ;;
bare)
    # not on the games disk: it is flat, and the shareware's own names
    # (README.TXT, SETUP.EXE) are other games' there too
    ;;
esac
