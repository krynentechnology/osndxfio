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
#include <stdlib.h>
#include <errno.h>
#include <mem.h>
#include <time.h>
#include <sys\timeb.h>

// ---- include files ----
#include <osdef.h>
#include <osfio.hpp>
#include <osndxfio.hpp>

// ---- local symbol definitions ----
#define MAX_NB_IDS          1000
#define MAX_NB_NAMES        100
#define MAX_NB_DEPARTMENTS  10
#define MAX_NB_RECORDS      50000
#define DATA_SIZE           200
#define SIZE_OF_NAME        10
#define SIZE_OF_DEPARTMENT  15

// ---- local data definitions ----
static BYTE generatedIds[ MAX_NB_IDS ];
static BYTE generatedNames[ MAX_NB_NAMES ];
static BYTE generatedDepartments[ MAX_NB_DEPARTMENTS ];
static U16 const maxRecords = U16( MAX_NB_RECORDS );

struct sTEST_OBJECT
{
    U32  id;
    char name[ SIZE_OF_NAME ];
    char department[ SIZE_OF_DEPARTMENT ];
    BYTE data[ DATA_SIZE ];
    
    sTEST_OBJECT() // Constructor.
    :
        id( 0 )
    {
        ::memset( name, 0, sizeof( name ));
        ::memset( department, 0, sizeof( department ));
        ::memset( data, 0, sizeof( data ));
    }
};

static sTEST_OBJECT testObjects[ maxRecords ];
#define OFFSET_DEPARTMENT ( SIZE_OF_NAME + sizeof( U32 ))     
#define OFFSET_NAME       sizeof( U32 )     
static OSNDXFIO::sKEY_SEGMENT key1[ 2 ] =
    { OSNDXFIO::sKEY_SEGMENT( OSNDXFIO::tBYTE, OFFSET_DEPARTMENT, SIZE_OF_DEPARTMENT ),
        OSNDXFIO::sKEY_SEGMENT( OSNDXFIO::tBYTE, OFFSET_NAME, SIZE_OF_NAME ) };
static OSNDXFIO::sKEY_SEGMENT key2[ 1 ] =
    { OSNDXFIO::sKEY_SEGMENT( OSNDXFIO::tU32, 0, sizeof( U32 )) };
static OSNDXFIO::sKEY_SEGMENT key3[ 2 ] =
    { OSNDXFIO::sKEY_SEGMENT( OSNDXFIO::tBYTE, OFFSET_NAME, SIZE_OF_NAME ),
        OSNDXFIO::sKEY_SEGMENT( OSNDXFIO::tU32, 0, sizeof( U32 )) };

static STRING database1 = "testDb1.dat";

static U32 passedCounter = 0;
static U32 failedCounter = 0;

// ---- local functions ----

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

    ::printf( "%02d:%02d:%02d.%03d OSNDXFIO T%-5d ",
    pLocalTime->tm_hour,
    pLocalTime->tm_min,
    pLocalTime->tm_sec,
    systemTime.millitm,
    testNumber );

    ::printf( "%s ", description );
}

/*============================================================================*/
void getNextObject( sTEST_OBJECT& object )
/*============================================================================*/
{
    ::memset( &object, 0, sizeof( object ));

    object.id = ::rand() % MAX_NB_IDS;
    generatedIds[ object.id ]++;

    int randomName = ::rand() % MAX_NB_NAMES;
    ::sprintf( object.name, "MY-NAME-%02d", randomName );
    generatedNames[ randomName ]++;

    int randomDepartment = ::rand() % MAX_NB_DEPARTMENTS;
    ::sprintf( object.department, "MY_DEPARTMENT-%d", randomDepartment );
    generatedDepartments[ randomDepartment ]++;
}

/**
 *  Test create and close empty database.
 *
 *  @return  True if successful.
 */
bool test1( void )
/*============================================================================*/
{
    printDescription( 1, "Create and close empty database" );

    OSNDXFIO::sKEY_SEGMENT key[ 2 ] =
        { OSNDXFIO::sKEY_SEGMENT( OSNDXFIO::tBYTE, 0, SIZE_OF_NAME  ),
        OSNDXFIO::sKEY_SEGMENT( OSNDXFIO::tBYTE, ( SIZE_OF_NAME - 1 ),
        SIZE_OF_DEPARTMENT ) }; // Overlap of data segments.

    OSNDXFIO::sKEY_DESC keyDesc[ 3 ];
    keyDesc[ 0 ].nrOfSegments = NR_ELEMENTS( key );
    keyDesc[ 0 ].apSegment    = key;
    keyDesc[ 1 ].nrOfSegments = NR_ELEMENTS( ::key2 );
    keyDesc[ 1 ].apSegment    = ::key2;
    keyDesc[ 2 ].nrOfSegments = NR_ELEMENTS( ::key3 );
    keyDesc[ 2 ].apSegment    = ::key2;

    (void)OSFIO::erase( database1 ); // If exist, erase test database.

    OSNDXFIO testDb;
    // Try to open non-existing database and check last error.
    bool statusOk = !testDb.open( database1 ); // Fails!
    statusOk = statusOk && ( testDb.getLastError() == OSNDXFIO::NO_DATABASE );
    // Try to create database with invalid key descriptor and check last error.
    statusOk = statusOk && !testDb.create( database1, NR_ELEMENTS( keyDesc ), keyDesc ); // Fails!
    statusOk = statusOk && ( testDb.getLastError() == OSNDXFIO::INVALID_KEY_DESCRIPTOR );

    keyDesc[ 0 ].nrOfSegments = NR_ELEMENTS( ::key1 );
    keyDesc[ 0 ].apSegment    = ::key1;
    // Create database with valid key descriptor and try to open it.
    statusOk = statusOk && testDb.create( database1, NR_ELEMENTS( keyDesc ), keyDesc );
    statusOk = statusOk && !testDb.open( database1 ); // Fails, database1 already open!
    statusOk = statusOk && ( testDb.getLastError() == OSNDXFIO::DATABASE_ALREADY_OPENED ); // Check last error.
    statusOk = statusOk && testDb.close();
    // Try to create same database again with valid key descriptor.
    statusOk = statusOk && !testDb.create( database1, NR_ELEMENTS( keyDesc ), keyDesc ); // Fails!
    statusOk = statusOk && ( testDb.getLastError() == OSNDXFIO::DATABASE_ALREADY_EXIST ); // Check last error.
    (void)testDb.close();

    return statusOk;
}

/**
 *  Test open empty database and create records.
 *
 *  @return  True if successful.
 */
bool test2( void )
/*============================================================================*/
{
    printDescription( 2, "Open empty database and create records" );

    OSNDXFIO testDb;
    // Open existing empty database.
    bool statusOk = testDb.open( database1 );

    // Clear testObject array;
    ::memset( testObjects, 0, sizeof( testObjects ));
    // Create maxRecords records.
    for ( U16 i = 0; ( statusOk && ( i < maxRecords )); i++ )
    {
        sTEST_OBJECT testObject;
        OSNDXFIO::sRECORD testRecord( sizeof( sTEST_OBJECT ), 0, sizeof( sTEST_OBJECT ), (BYTE*)&testObject );
        getNextObject( testObject );
        testObjects[ i ] = testObject;

        U32 index = 0;
        statusOk = testDb.createRecord( testRecord, index );
    }

    statusOk = statusOk && ( testDb.getNrOfRecords() == maxRecords );
    statusOk = statusOk && testDb.close();
    (void)testDb.close();

    return statusOk;
}

/**
 *  Test created database and read all records.
 *
 *  @return  True if successful.
 */
bool test3( void )
/*============================================================================*/
{
    printDescription( 3, "Open created database and read all records" );

    OSNDXFIO testDb;
    // Open created database.
    bool statusOk = testDb.open( database1 );
    sTEST_OBJECT testObject;
    OSNDXFIO::sRECORD testRecord( sizeof( sTEST_OBJECT ), 0, 0, (BYTE*)&testObject );
    U32 nbRecords = 0;

    // Read all records and compare.
    for ( U32 i = 0; ( statusOk && ( i < testDb.getNrOfRecords() )); i++ )
    {
        testRecord.dataSize = 0;
        ::memset( &testObject, INVALID_VALUE, sizeof( testObject ));
        statusOk = testDb.getRecord( i, testRecord );
        statusOk = statusOk && ( U32( sizeof( sTEST_OBJECT )) == testRecord.dataSize );
        // Compare with testObject array.
        statusOk = statusOk && ( ::memcmp( &testObject, &testObjects[ i ], sizeof( testObject )) == 0 );

        if ( statusOk )
        {
            nbRecords++;
        }
    }

    statusOk = statusOk && ( testDb.getNrOfRecords() == nbRecords );
    statusOk = statusOk && testDb.close();
    (void)testDb.close();

    return statusOk;
}

/**
 *  Test retrieving records by (partial) key.
 *
 *  @return  True if successful.
 */
bool test4( void )
/*============================================================================*/
{
    printDescription( 4, "Retrieving records by (partial) key" );

    OSNDXFIO testDb;
    // Open created database.
    bool statusOk = testDb.open( database1 );
    sTEST_OBJECT testObject;
    OSNDXFIO::sRECORD testRecord( sizeof( sTEST_OBJECT ), 0, 0, (BYTE*)&testObject );
    U32 nbRecords = 0;

    for ( U16 i = 0; ( statusOk && ( i < MAX_NB_IDS )); i++ )
    {
        if ( generatedIds[ i ] )
        {
            U32 searchId = i;
            OSNDXFIO::sKEY key( 1, sizeof( searchId ), (BYTE*)&searchId ); // 1 == key2.
            U32 index = INVALID_VALUE;
            statusOk = testDb.existRecord( key, index );
            statusOk = statusOk && testDb.getRecord( index, testRecord );
            // Compare with testObject array.
            statusOk = statusOk && ( ::memcmp( &testObject, &testObjects[ index ], sizeof( testObject )) == 0 );

            if ( statusOk )
            {
                U32 recordCount = testDb.getSearchCount( key );
                for ( U32 j = 1; ( statusOk && ( j < recordCount )); j++ )
                {
                    statusOk = testDb.getNextRecord( 1, testRecord, index ); // 1 == key2.
                    // Compare with testObject array.
                    statusOk = statusOk && ( ::memcmp( &testObject, &testObjects[ index ], sizeof( testObject )) == 0 );
                }

                nbRecords += recordCount;
            }
        }
    }

    statusOk = statusOk && ( testDb.getNrOfRecords() == nbRecords );
    nbRecords = 0;
    // Partial key retrieval.
    for ( U16 i = 0; ( statusOk && ( i < MAX_NB_DEPARTMENTS )); i++ )
    {
        if ( generatedDepartments[ i ] )
        {
            BYTE searchKey[ SIZE_OF_DEPARTMENT + SIZE_OF_NAME ];
            ::memset( searchKey, 0, sizeof( searchKey ));
            ::sprintf( (char*)searchKey, "MY_DEPARTMENT-%d", i );
            OSNDXFIO::sKEY key( 0, SIZE_OF_DEPARTMENT, searchKey ); // 0 == key1.
            U32 index = INVALID_VALUE;
            statusOk = testDb.existRecord( key, index );
            statusOk = statusOk && testDb.getRecord( index, testRecord );
            // Compare with testObject array.
            statusOk = statusOk && ( ::memcmp( &testObject, &testObjects[ index ], sizeof( testObject )) == 0 );

            if ( statusOk )
            {
                U32 recordCount = testDb.getSearchCount( key );
                for ( U32 j = 1; ( statusOk && ( j < recordCount )); j++ )
                {
                    statusOk = testDb.getNextRecord( 0, testRecord, index ); // 0 == key1.
                    // Compare with testObject array.
                    statusOk = statusOk && ( ::memcmp( &testObject, &testObjects[ index ], sizeof( testObject )) == 0 );
                }

                nbRecords += recordCount;
            }
        }
    }

    statusOk = statusOk && ( testDb.getNrOfRecords() == nbRecords );
    statusOk = statusOk && testDb.close();
    (void)testDb.close();

    return statusOk;
}

/*============================================================================*/
int main()
/*============================================================================*/
{
    time_t startTime;
    ::time( &startTime );
    ::printf( "OSNDXFIO TEST started at %s\n", ::ctime( &startTime ));
    ::printf( "OSNDXFIO size of type eERROR = %d\n", sizeof( OSNDXFIO::eERROR ));
    ::printf( "OSNDXFIO size of type eTYPE = %d\n", sizeof( OSNDXFIO::eTYPE ));
    ::printf( "OSNDXFIO size of type sKEY_SEGMENT = %d\n", sizeof( OSNDXFIO::sKEY_SEGMENT ));
    ::printf( "OSNDXFIO size of type sKEY_DESC = %d\n", sizeof( OSNDXFIO::sKEY_DESC ));
    ::printf( "OSNDXFIO size of type sRECORD = %d\n\n", sizeof( OSNDXFIO::sRECORD ));

    printResult( test1());
    printResult( test2());
    printResult( test3());
    printResult( test4());

    ::printf( "\nOSNDXFIO TEST %d passed, %d failed, stopped at %s\n\n", passedCounter, failedCounter, ::ctime( &startTime ));

    // WAIT_FOR_KEYPRESSED;
    return 0;
}

