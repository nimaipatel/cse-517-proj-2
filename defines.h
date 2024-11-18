#ifndef _DEFINES_H
#define _DEFINES_H

#include <stddef.h>
#include <stdint.h>

typedef char Char8;

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;

typedef signed char S8;
typedef signed short S16;
typedef signed int S32;
typedef signed long long S64;

typedef uintptr_t Word;

static_assert(sizeof(void *) == sizeof(Word), "");
static_assert(sizeof(size_t) == sizeof(Word), "");

static_assert(sizeof(U8) == 1, "");
static_assert(sizeof(U16) == 2, "");
static_assert(sizeof(U32) == 4, "");
static_assert(sizeof(U64) == 8, "");

static_assert(sizeof(S8) == 1, "");
static_assert(sizeof(S16) == 2, "");
static_assert(sizeof(S32) == 4, "");
static_assert(sizeof(S64) == 8, "");

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#endif // _DEFINES_H
