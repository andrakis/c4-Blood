// io.h -- MSVC/Watcom low-level files on a Unix libc
#ifndef C4BLOOD_IO_H
#define C4BLOOD_IO_H 1
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "c4blood.h"
#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef O_TEXT
#define O_TEXT 0
#endif
#ifndef S_IREAD
#define S_IREAD  0400
#define S_IWRITE 0200
#endif
#endif
