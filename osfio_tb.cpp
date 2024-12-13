/**
 *  Copyright (C) 2024, Kees Krijnen.
 *
 *  This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program. If not, see <https://www.gnu.org/licenses/> for a copy.
 *
 *  License: GPL, v3, as defined and found on www.gnu.org,
 *           https://www.gnu.org/licenses/gpl-3.0.html
 */

// ---- system includes ----
#include <stdio.h>
#include <errno.h>
#include <mem.h>
#include <time.h>
#include <sys\timeb.h>

// ---- application includes ----
#include <osfio.hpp>

// ---- symbol definitions ----
#define DATA_SIZE 1024

// ---- data definitions ----
STRING  file_name = "TEST.DB";
OSFIO   handle;
BYTE    test_data1[ DATA_SIZE ];
BYTE    test_data2[ DATA_SIZE ];
U32     file_size     = 0;
U32     file_pointer  = 0;
U32     passedCounter = 0;
U32     failedCounter = 0;

// ---- functions ----

/*============================================================================*/
void printResult( bool passed )
/*============================================================================*/
{
    ::printf( "- %s\n", passed ? "Passed" : "Failed" );
    
    if ( passed )
    {
      passedCounter++;
    }
    else
    {
      failedCounter++;
    }
}

/*============================================================================*/
void printDescription( U16 testNumber, const STRING description )
/*============================================================================*/
{
    static struct timeb systemTime;
    struct tm*          pLocalTime;
    
    ::ftime( &systemTime );
    pLocalTime = ::localtime( &systemTime.time );
    
    ::printf( "%02d:%02d:%02d.%03d OSFIO T%-5d ",
      pLocalTime->tm_hour,
      pLocalTime->tm_min,
      pLocalTime->tm_sec,
      systemTime.millitm,
      testNumber );
    
    ::printf( "%s ", description );
}

/*
 *  Test create, open, close and delete file.
 *
 *  @return  True if successful.
 */
bool test1( void )
/*============================================================================*/
{
    printDescription( 1, "Create, open, close and delete file" );

    // check for file exist
    bool status_ok = !handle.open( file_name, READ_ONLY_ACCESS );

    if ( !status_ok )
    {
        status_ok = handle.close();
        status_ok = status_ok && handle.erase( file_name );
        status_ok = status_ok && !handle.open( file_name, READ_ONLY_ACCESS );
    }

    status_ok = status_ok && handle.create( file_name );

    status_ok = status_ok && handle.close();

    status_ok = status_ok && handle.open( file_name );

    status_ok = status_ok && ( handle.timestamp() != (U32)INVALID_VALUE );

    status_ok = status_ok && handle.close();

    handle.close(); // close anyway

    status_ok = status_ok && handle.erase( file_name );

    return status_ok;
}

/*
 *  Test read and write file.
 *
 *  @return  True if successful.
 */
bool test2( void )
/*============================================================================*/
{
    printDescription( 2, "Read and write file" );

    bool status_ok = !handle.open( file_name, READ_ONLY_ACCESS );

    if ( !status_ok )
    {
        status_ok = handle.close();
        status_ok = status_ok && handle.erase( file_name );
        status_ok = status_ok && !handle.open( file_name, READ_ONLY_ACCESS );
    }

    status_ok = status_ok && handle.create( file_name );

    status_ok = status_ok && handle.close();

    status_ok = status_ok && handle.open( file_name );

    for ( int i = 0; i < sizeof( test_data1 ); i++ )
    {
        test_data1[ i ] = (BYTE)i;
    }

    memset( test_data2, 0, DATA_SIZE );

    status_ok = status_ok && (( file_pointer = handle.position() ) !=
        (U32)INVALID_VALUE );

    status_ok = status_ok && ( file_pointer == 0);

    status_ok = status_ok && handle.write( test_data1, DATA_SIZE );

    status_ok = status_ok && (( file_size = handle.size() ) !=
      (U32)INVALID_VALUE );

    status_ok = status_ok && ( file_size == DATA_SIZE );

    status_ok = status_ok && (( file_pointer = handle.position() ) !=
      (U32)INVALID_VALUE );

    status_ok = status_ok && ( file_size == file_pointer );

    status_ok = status_ok && handle.read( 0, test_data2, DATA_SIZE );

    status_ok = status_ok && ( memcmp( test_data1, test_data2, DATA_SIZE ) == 0 );

    status_ok = status_ok && handle.eof();

    status_ok = status_ok && !handle.read( test_data2, 1 );

    handle.close(); // close anyway

    return status_ok;
}

/*
 *  Test append write.
 *
 *  @return  True if successful.
 */
bool  test3( void )
/*============================================================================*/
{
    printDescription( 3, "Append write file" );

    bool status_ok = handle.open( file_name, READ_WRITE_ACCESS );

    status_ok = status_ok && (( file_pointer = handle.position() ) !=
        (U32)INVALID_VALUE );

    status_ok = status_ok && ( file_pointer == 0 );

    status_ok = status_ok && handle.write( EOF_POSITION, test_data1, DATA_SIZE );

    status_ok = status_ok && (( file_size = handle.size() ) !=
        (U32)INVALID_VALUE );

    status_ok = status_ok && ( file_size == ( 2 * DATA_SIZE ) );

    status_ok = status_ok && (( file_pointer = handle.position() ) !=
        (U32)INVALID_VALUE );

    status_ok = status_ok && ( file_size == file_pointer );

    status_ok = status_ok && handle.read( DATA_SIZE, test_data2, DATA_SIZE );

    status_ok = status_ok && ( memcmp( test_data1, test_data2, DATA_SIZE ) == 0 );

    status_ok = status_ok && handle.eof();

    status_ok = status_ok && !handle.read( test_data2, 1 );

    status_ok = status_ok && handle.truncate( DATA_SIZE );

    status_ok = status_ok && (( file_size = handle.size() ) !=
        (U32)INVALID_VALUE );

    status_ok = status_ok && ( file_size == DATA_SIZE );

    status_ok = status_ok && (( file_pointer = handle.position() ) !=
        (U32)INVALID_VALUE );

    status_ok = status_ok && ( file_size == file_pointer );

    status_ok = status_ok && !handle.truncate( DATA_SIZE );

    status_ok = status_ok && !handle.read( test_data2, 1 );

    handle.close(); // close anyway

    return status_ok;
}

int main()
{
    time_t startTime;
    ::time( &startTime );
    ::printf( "OSFIO TEST started at %s\n", ::ctime( &startTime ) );
    ::printf( "OSFIO TEST started at %s\n", ctime( &startTime ));
    ::printf( "OSDEF size of type bool = %d\n", sizeof( bool ));
    ::printf( "OSDEF size of type U32 = %d\n", sizeof( U32 ));
    ::printf( "OSDEF size of type STRING = %d\n", sizeof( STRING ));
    ::printf( "OSDEF size of type POINTER = %d\n\n", sizeof( POINTER ));

    printResult( test1() );
    printResult( test2() );
    printResult( test3() );
    handle.erase( file_name );

    ::printf( "\nOSFIO TEST %d passed, %d failed, stopped at %s\n\n",
      passedCounter, failedCounter, ::ctime( &startTime ));

    // WAIT_FOR_KEYPRESSED;
    return 0;
}

