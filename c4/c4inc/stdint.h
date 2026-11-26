// stdint.h for c4lc (32-bit): Duke's, with the fast and least types opl3.c uses
#ifndef C4BLOOD_STDINT_H
#define C4BLOOD_STDINT_H 1
typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef int                int32_t;
typedef unsigned int       uint32_t;
typedef long long          int64_t;
typedef unsigned long long uint64_t;
typedef int                intptr_t;
typedef unsigned int       uintptr_t;
typedef int8_t   int_fast8_t;   typedef uint8_t  uint_fast8_t;
typedef int32_t  int_fast16_t;  typedef uint32_t uint_fast16_t;
typedef int32_t  int_fast32_t;  typedef uint32_t uint_fast32_t;
typedef int64_t  int_fast64_t;  typedef uint64_t uint_fast64_t;
typedef int8_t   int_least8_t;  typedef uint8_t  uint_least8_t;
typedef int16_t  int_least16_t; typedef uint16_t uint_least16_t;
typedef int32_t  int_least32_t; typedef uint32_t uint_least32_t;
#define INT8_MIN   (-128)
#define INT16_MIN  (-32768)
#define INT32_MIN  (-2147483647 - 1)
#define INT8_MAX   127
#define INT16_MAX  32767
#define INT32_MAX  2147483647
#define UINT8_MAX  255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295u
#define INTPTR_MAX INT32_MAX
#define INT64_C(c)  c##LL
#define UINT64_C(c) c##ULL
#endif
