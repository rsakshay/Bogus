#ifndef GLOBALS_H
#define GLOBALS_H

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

constexpr uint16 max_uint8 = 0xFFu;
constexpr uint16 max_uint16 = 0xFFFFu;
constexpr uint32 max_uint32 = 0xFFFFFFFFu;
constexpr uint64 max_uint64 = 0xFFFFFFFFFFFFFFFFull;

constexpr int8 max_int8 = 0x7Fu;
constexpr int16 max_int16 = 0x7FFFu;
constexpr int32 max_int32 = 0x7FFFFFFFu;
constexpr int64 max_int64 = 0x7FFFFFFFFFFFFFFFu;

constexpr int8 min_int8 = ~max_int8;
constexpr int16 min_int16 = ~max_int16;
constexpr int32 min_int32 = ~max_int32;
constexpr int64 min_int64 = ~max_int64;

#define MIN( a, b ) ( a < b ) ? a : b
#define MAX( a, b ) ( a > b ) ? a : b
#define CLAMP( x, a, b ) MIN( MAX( x, a ), b )

#define ALIGNUP_POW2( xVar, xPow2 ) ( ( xVar + ( xPow2 - 1 ) ) & ( ~( xPow2 - 1 ) ) )
#define ALIGNOF( T ) __alignof( T )

#define BYTES( n ) ( n )
#define KILOBYTES( n ) ( n << 10 )
#define MEGABYTES( n ) ( n << 20 )
#define GIGABYTES( n ) ( (uint64)n << 30 )
#define TERABYTES( n ) ( (uint64)n << 40 )

#endif
