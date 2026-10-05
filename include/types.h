#ifndef BFBB_TYPES_H
#define BFBB_TYPES_H

#include "macros.h"

#ifdef XBOX
#include <stddef.h>
#endif

// Note: only include this header inside BFBB-related headers/source code files.
// Don't include this in any RenderWare, system, bink, etc. files

#if defined(GAMECUBE) || defined(PS2) || defined(XBOX)
typedef signed char S8;
typedef signed short S16;
typedef signed int S32;
#ifdef _MSC_VER
typedef signed __int64 S64;
#else
typedef signed long long S64;
#endif

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
#ifdef _MSC_VER
typedef unsigned __int64 U64;
#else
typedef unsigned long long U64;
#endif

typedef float F32;
typedef double F64;
#endif

#ifdef NULL
#undef NULL
#endif
#define NULL 0

#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

// Preserve __declspec for CodeWarrior section/weak declarations and native
// MSVC declarations; stub it only for compilers that cannot parse it.
#if !defined(__MWERKS__) && !defined(_MSC_VER)
#define __declspec(x)
// #define asm
#endif

#define WEAK __declspec(weak)

typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
#ifdef _MSC_VER
typedef signed __int64 s64;
#else
typedef signed long long s64;
#endif
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
#if defined(PS2)
typedef unsigned int size_t;
#elif !defined(XBOX)
typedef unsigned long size_t;
#endif
#ifdef _MSC_VER
typedef unsigned __int64 u64;
#else
typedef unsigned long long u64;
#endif

typedef unsigned short ushort;
typedef unsigned int uint;

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;
typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;

typedef float f32;
typedef double f64;
typedef volatile f32 vf32;
typedef volatile f64 vf64;

typedef int BOOL;

typedef int unknown;

#ifndef __cplusplus
typedef unsigned short wchar_t;
typedef wchar_t wint_t;
#endif

// Basic defines to allow newer-like C++ code to be written
#define TRUE 1
#define FALSE 0

#define null 0

#ifndef NULL
#define NULL 0
#endif

#define UINT32_MAX 0xffffffff

#endif // !TYPES_H
