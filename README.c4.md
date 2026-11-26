# Blood on c4

c4's port of Blood (Monolith, 1997). The base is
[chocolate-blood](https://github.com/NBlood/chocolate-blood) (`master`, the
`upstream` remote): nukeykt's Blood-RE decompilation of Blood 1.21's game
code, which is C++, on portable C layers for the engine (Ken Silverman's
Build), Apogee's audiolib and 3D Realms' MACT. c4's work is on the `c4`
branch, in commits of its own.

c4's C compiler, c4lc, has no C++. The plan
(`~/gate/c4cxx/plan.md` on helios, to become a c4 doc) is a translator from C++
to C, c4cxx, whose output c4lc compiles. Until then, this branch has the
host reference build and the c4 platform.

## State (2026-11-26)

- **The host build (B0)**: `c4/host.sh` builds the game with gcc and g++
  `-m32`: 113 units, all of them. The SDL platform
  (`source/build/system.c`, `video.c` and `sound.c`) is replaced by
  `c4/blood-host.c`: no display, no sound, and virtual time, which moves
  1 ms each time the game reads a clock. Every frame is checksummed, and
  two runs give the same checksums. This is the reference that the c4
  builds will be held to.
- **`char` is unsigned** (`-funsigned-char`), as Watcom's was. Blood and
  its engine rely on it: `cache1d`'s `filegrp[]` holds 255 for "not in a
  group", and the fire effect sums bytes.
- **Versions.** chocolate-blood is Blood-RE's **Plasma Pak 1.21**, with
  Blood-RE's version switches removed. It does not run other versions'
  data:
  - shareware 1.11 has `SHARE000.ART` rather than `TILES###.ART`, and no
    tile fonts at 4096 and up (1.20 added them; 1.11 has QFN fonts). With
    the art name patched in (not committed), it reaches the main menu
    with its blood drips but no text;
  - registered 1.00 lacks QAVs 118-124.

  Shareware 1.11 needs Blood-RE's `SHAREWARE` and `APPVER_BLOODREV` cases
  for version BS111A put back.
- **c4lc** compiles 34 of the 35 C units (engine, MACT, audiolib) with the
  headers in `c4/inc` and `c4/c4inc`. `source/build/a_asm.c` needs an
  anonymous struct inside a union, which c4lc does not have. c4lc's
  `char` is signed, and there is no option yet to make it unsigned.
- **The game's configuration** must ask for VESA (`ScreenMode = 1`). Mode
  2 writes to the VGA memory at `0xA0000`, which only DOS has.

## Files

| | |
|---|---|
| `c4/host.sh` | the host build. Output goes to `out/host/blood-host` |
| `c4/blood-host.c` | the host platform: PIT, keyboard, VGA frame, no sound, and checksums. Its options are `-sum N`, `-frames N`, `-shot N` and `-keys FILE` |
| `c4/c4blood.c`, `c4/inc/*.h` | what MSVC's and Watcom's C libraries gave the game: `_dos_findfirst`, `_splitpath2`, `itoa`, `strupr`, `filelength`, `io.h`, `conio.h`, and an SDL3 header with the atomics and mutexes outside the platform files |
| `c4/c4inc/stdint.h` | for c4lc only |
| `c4/cxxrt.cpp` | `new`, `delete`, the pure-virtual trap and static guards, without a C++ library |
| `c4/fetch-shareware.sh` | fetches Blood 1.11 shareware (17,349,441 bytes, checked by its SHA-256) and unpacks it to `data/blood111` |
| `c4/tools/` | `unshr.py` (the shareware installer's archive), `includefix.py` (include spelling), `protofix.py` (definitions to match prototypes) |

## Data

Nothing of the game's data is committed (`.gitignore`: `data/`, `out/`).

- **The shareware** (episode 1, version 1.11) comes from
  `c4/fetch-shareware.sh`. BLOODHLP.TXT says: "You may freely distribute
  Blood shareware provided that you include all the original files and
  that you do not charge for distribution." So a disk that carries it
  carries every file of the original set.
- **The full game** is the person's own files.

## Licences

- Build engine: `BUILDLIC.TXT` (Ken Silverman: non-commercial, with
  credit).
- audiolib and MACT: GPL-2, from 3D Realms' releases.
- The game code is nukeykt's decompilation. Its GPL-2 notices are the
  decompiler's. There is no grant from Monolith or Atari, who hold Blood.
