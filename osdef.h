#ifndef OSDEF_H
#define OSDEF_H
/**
 *  Copyright (C) 2024, Kees Krijnen.
 *
 *  This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published by the
 *  Free Software Foundation, either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/> for a
 *  copy.
 *
 *  License: LGPL, v3, as defined and found on www.gnu.org,
 *           https://www.gnu.org/licenses/lgpl-3.0.html
 *
 *  Description: Type, symbol and mocro definitions
 */

// ---- include files ----
#ifdef __CPPBUILDERIDE__
#ifdef __cplusplus
using namespace std;
#endif
#endif

// ---- type definitions ----
typedef void                  VOID_FUNC();

typedef unsigned char         BYTE;
typedef short                 S16;
typedef unsigned short        U16;
typedef long                  S32;
typedef unsigned long         U32;
typedef long int              S64;
typedef unsigned long int     U64;
typedef float                 R32;
typedef double                R64;
typedef long double           R80;

// ---- symbol definitions ----
#define STRING                char*
#define POINTER               void*
#define HANDLE                void*
#ifdef  NULL
  #undef NULL
#endif
#define NULL                  0
#define NO_FUNC               0    // Function NULL pointer
#define ENUM_MIN              (-1)
#define ENUM_FIRST            0
#define INVALID_VALUE         (-1)
#if !( defined( CPU_BIG_ENDIAN ) || defined( CPU_LITTLE_ENDIAN ))
  #define CPU_LITTLE_ENDIAN        // Default little endian CPU
#endif

// ---- macro definitions ----
#define MAX( a, b )           ( ( (a) > (b) ) ? (a) : (b) )
#define MIN( a, b )           ( ( (a) < (b) ) ? (a) : (b) )
#define ABS( a )              ( ( (a) >= 0 ) ? (a) : -(a) )
#define NEG( a )              ( ( (a) <= 0 ) ? (a) : -(a) )
#define BOUND( a, b, c )      ( ( (b) < (a) ) ? (a) : ( ( (b) > (c) ) ? (c) : (b) ) )
#define SIGN( a )             ( ( (a) >= 0 ) ? 1 : (-1) )
#define IS_BOUNDED( a, b, c)  ( ( ( (a) >= (b) ) && ( (a) <= (c) ) ) ? true : false )
#define IN_RANGE( a, b, c )   ( ( ( (a) > (b) ) && ( (a) < (c) ) ) ? true : false )
#define IS_ODD( x )           ( ( (x) & 1 ) != 0 )
#define IS_EVEN( x )          ( ( (x) & 1 ) == 0 )
#define ODD_EVEN( x )         ( (x) & 1 )
#define MEMEQ( a, b, c )      ( memcmp( (a), (b), (c) ) == 0 )
#define MEMEQC( a, b )        ( memcmp( (a), (b), ( sizeof (b) - 1 ) ) == 0 )
#define NR_ELEMENTS( a )      ( sizeof ( (a) ) / sizeof ( ( (a)[ 0 ] ) ) )
#define LENGTH( a )           ( sizeof ( (a) ) - 1 )
#define SUCCESSFUL_RETURN     return true
#define UNSUCCESSFUL_RETURN   return false
#define WAIT_FOR_KEYPRESSED   getch()

#endif /* OSDEF_H */

