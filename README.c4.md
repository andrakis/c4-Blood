# Blood on c4

c4's port of Blood (Monolith, 1997), on the `c4` branch. `master` is
[chocolate-blood](https://github.com/NBlood/chocolate-blood) (remote
`upstream`).

- **The game code** (`source/blood`) is
  [Blood-RE](https://github.com/nukeykt/Blood-RE) at 4d4869a: nukeykt's
  matching decompilation of every DOS version from shareware 1.10 to 1.21,
  with its version switches (`APPVER_EXEDEF`). It is ported to gcc and to
  chocolate-blood's layers by the commits after the import.
- **The engine, MACT and audiolib** (`source/build`, `mact`, `audiolib`)
  and **qtools** (`source/blood/qtools`) are chocolate-blood's portable C
  and C++ in place of Watcom's libraries. qtools is version 1.11A, the one
  that BS111A and BP121 use.

c4's C compiler, c4lc, has no C++: c4cxx, c4's translator from C++ to C
(c4's `src/c4cxx`, `docs/c4cxx-plan.md`), makes C of Blood's C++ that c4lc
compiles.

## State (2026-11-28)

- **On c4** Blood plays from C4IX's desktop (Go > Games > Blood, or `blood`
  in a Terminal) with its picture, sound, music, keyboard and mouse: c4's
  build of the game, `c4.mk` and `c4/c4-objs.sh` (every C unit through c4lc,
  every C++ unit through c4cxx and then c4lc), with c4's platform
  (`c4/blood-c4.c` and the files below), linked for C4IX by `c4-images.sh`
  with the shareware beside it, all its files, in `/usr/games/blood`.
- **`test-blood`** (c4's `make test-blood`, `c4/test-blood.sh`): that build
  plays the shareware's demo loop inside C4IX as the host build (B0) does,
  every frame's checksum B0's, on its virtual clock (a test: `-sum`). c4pc's
  `test-c4pc` starts it from the desktop, sees it draw its menu, quits it by
  the keyboard and hears its sound. c4cxx's `blood-c4lc.sh` holds the
  headless build to B0 on the bare machine, under C4DOS and in C4IX.
- **Speed**: about 45 frames a second of the demo loop on a machine of 200 M
  c4 instructions a second (c4pc-host `-hz 200`; 13 without the drawing
  accelerators, 2026-11-28). Two sets of accelerators, where the machine
  has them, each with the C's results:
  - the 64-bit arithmetic (`PRAGMAS.H`, qtools' `misc.h`) by Shadow
    Warrior's (`c4/inc/c4blood.h`: `C4_MULSCALE` and the rest);
  - the engine's drawing loops, `a_asm.c`'s `hlineasm4`, `vlineasm4`,
    `mvlineasm4`, `vlineasm1` and `mvlineasm1`, by Blood's own
    (`c4/accel.mjs`, and c4pc's `src/c4pc/games/blood.c`;
    `c4/inc/c4accel.h`). These keep A.ASM's packed coordinates as
    `a_asm.c` keeps them, which Shadow Warrior's loops do not, and in C
    their 64-bit steps were calls to c4lc's long long routines.
    `c4/acceltest.c` holds each to its loop (`test-blood` runs it on
    libjs and c4pc-host).

- **`c4/host.sh`** builds Blood with gcc and g++ `-m32`: 31 C units and 82
  game units. `c4/blood-host.c` replaces chocolate-blood's SDL platform with
  one thread, no display, no sound and virtual time, and checksums every
  frame. Two runs give the same checksums.
- **`BLOOD_VER`** chooses the version: `BS111A` (the default, shareware 1.11
  of 23 September 1997) or `BP121` (Plasma Pak 1.21), or any other
  Blood-RE builds.
- **The shareware runs its attract demos** behind the main menu, as DOS
  Blood does: the cemetery, pickups, TNT. That they are in sync with DOS
  Blood frame for frame is not yet checked against a DOS run.
- **Not working**: sound in the host build (it has none); Smacker cutscenes (no decoder:
  `c4/smacker.c` fails to open them, as when the CD is missing); CD audio;
  network play (`c4/netnone.cpp`: one player).

## What Watcom gave Blood that gcc needs told

Each item is here because the game broke without it:

| Watcom | here | what broke |
|---|---|---|
| `char` is unsigned | `-funsigned-char` | the file cache's "not in a group" (255); the fire |
| 1-byte structure packing | `#pragma pack(push, 1)` in each Blood source (`c4/tools/packall.py`) | the demo header, `INPUT`, saves, maps |
| an enum is the smallest type that holds it | `-fshort-enums` (Blood's units only) | `GAMEOPTIONS`, 373 bytes |
| a bit-field takes a unit of its declared type | `-mms-bitfields` (Blood's units only) | `INPUT`, 22 bytes |
| `_MAX_PATH` is 144 | `c4/inc/c4blood.h` | `GAMEOPTIONS`' two paths |
| a `for` variable lives to the end of the block | the declaration moved before the `for` (`c4/tools/forscope.py`, 34 loops) | a compile error |
| implicit `int`, and falling off the end returns whatever `eax` held | `CWeather`'s functions made `void` | gcc took the missing return as unreachable |
| the linker takes the game's definition over the library's | `NAME_replace` and `replace_hook()` | duplicate symbols |
| global constructors run in link order | `loadsave.o` links first | the savers link onto `LoadSave::head` |
| DOS file names have no case | `c4blood_open`, `fopen`, `access` | `tables.dat` for `TABLES.DAT` |

c4cxx and c4lc must do the same (plan §3).

## Files

| | |
|---|---|
| `c4/host.sh` | the host build. Output goes to `out/host-$BLOOD_VER/blood-host` |
| `c4/blood-host.c` | the host platform: the PIT, keyboard, VGA frame, no sound, and checksums. Its options are `-sum N`, `-frames N`, `-shot N` and `-keys FILE`. c4's headless build (c4cxx's tests) is the same |
| `c4/blood-c4.c` | c4's platform: `blood-host.c` built with `C4BLOOD_SHOW`, its time the machine's, its pages on the display, its keys and mouse from it (the browser's key codes as the PC's scan codes, into the keyboard interrupt). Given `-sum`, `-frames`, `-shot` or `-keys` it is a test, on `blood-host.c`'s virtual clock, with no sound; `-show` draws a test's pages too, `-window` plays in a window, `-trace` prints a line a second |
| `c4/blood-gui.c`, `blood-c4ix.c`, `blood-bare.c`, `blood-plat.h` | libjs's display (as Shadow Warrior's `sw-gui.c`): on C4IX the desktop gives it way until the game ends, or a window; C4IX's starts the game in `/usr/games/blood` |
| `c4/blood-snd.c` | the sound card: audiolib's `FX_` on its sixteen sample channels (in place of `FX_MAN.C` and its mixer), and the music, audiolib's MIDI player and AdLib driver, on its OPL, run ahead of the card's stream by `snd_wait` |
| `c4/BLOOD.CFG` | c4's setup: VESA at 320x200, the effects on the card (`FXDevice = 0`) and the music on its OPL as an AdLib (`MusicDevice = 3`) |
| `c4.mk`, `c4-images.sh`, `c4ix.vfs.txt`, `c4/c4-objs.sh`, `c4/test-blood.sh` | c4's build and test of the game (c4's `games/README.md`) |
| `c4/accel.mjs`, `c4/inc/c4accel.h`, `c4/acceltest.c` | Blood's accelerators for libjs (c4pc's are its `src/c4pc/games/blood.c`): `a_asm.c`'s five drawing loops, one instruction each, and the test that holds them to the loops |
| `c4/c4blood.c`, `c4/inc/*.h` | what MSVC's and Watcom's C libraries and DOS gave the game: `dos.h`'s file search, `io.h`, `conio.h`, `i86.h`, `_splitpath2`, `itoa`, file names without case, and an SDL3 header with only the clocks' atomics, the sound mutex and a delay |
| `c4/smacker.c`, `c4/inc/smack.h`, `svga.h` | Smacker without a decoder |
| `c4/netnone.cpp` | the network drivers (Mpath, HEAT, TEN) when there is no network, and DPMI memory for the CD driver |
| `c4/cxxrt.cpp` | `new`, `delete`, the pure-virtual trap and static guards, without a C++ library |
| `c4/c4inc/stdint.h` | for c4lc only |
| `c4/fetch-shareware.sh` | fetches Blood 1.11 shareware (17,349,441 bytes, checked by its SHA-256) and unpacks it to `data/blood111` |
| `c4/tools/` | `unshr.py` (the shareware installer's archive), `forscope.py`, `packall.py`, `verresolve.py` (resolves Blood-RE's version switches for one version, to compare trees), `includefix.py`, `incslash.py`, `protofix.py` |
| `c4/tools/port/` | the scripts that turned the Blood-RE import into the port commit, kept as its record, to apply again to a newer Blood-RE |

## Running

On c4: `make blood-data c4bb-images` in c4 with this as `games/blood`, then
`blood` in C4IX (the desktop's Go > Games > Blood). The host build: make a
directory with the game's files, and put `BLOOD.CFG` in it with
`ScreenMode = 1` (VESA; mode 2 writes to `0xA0000`) and `FXDevice = 13`,
`MusicDevice = 13` (none). Then:

    out/host-BS111A/blood-host -sum 100 -frames 3000 [-shot 300]

## Data

Nothing of the game's data is committed (`.gitignore`: `data/`, `out/`).

- **The shareware** (episode 1, version 1.11) comes from
  `c4/fetch-shareware.sh`. BLOODHLP.TXT says: "You may freely distribute
  Blood shareware provided that you include all the original files and
  that you do not charge for distribution." So a disk that carries it
  carries every file of the original set.
- **The full game** is the person's own files. Blood-RE covers 1.10 to
  1.21. 1.00 needs Monolith's patches first.

## Licences

- Build engine: `BUILDLIC.TXT` (Ken Silverman: non-commercial, with
  credit).
- audiolib and MACT: GPL-2, from 3D Realms' releases.
- The game code is nukeykt's decompilation. Its GPL-2 notices are the
  decompiler's. There is no grant from Monolith or Atari, who hold Blood.
