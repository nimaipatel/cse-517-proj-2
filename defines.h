#include <stddef.h>

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef U64 Word;
static_assert(sizeof(void *) == sizeof(Word), "");
static_assert(sizeof(size_t) == sizeof(Word), "");

#define MAX(a, b) ((a) > (b) ? (a) : (b))