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
 *  Description:
 *
 *  The OSNDXFIO module is a set of functions and definitions to setup a low
 *  level database (no query language). It is similar to the VMS operating
 *  system indexed file I/O functionality.
 *
 *  The OSNDXFIO package opens files without exclusive access and provides no
 *  locking/synchronization mechanisms for read/write. Hence, the applications
 *  using OSNDXFIO services for database access should use proper
 *  synchronization mechanisms. Otherwise, data integrity is not guaranteed.
 *
 *  The OSNDXFIO module creates, rebuilds, opens, closes, and deletes databases.
 *  The OSNDXFIO module reads, writes, updates, seeks, and deletes data objects.
 *  The OSNDXFIO module is responsible for providing indexing mechanism and
 *  defines an index structure that is generic. Applications can define their
 *  own index structures, known as the search key in the OSNDXFIO context. There
 *  is a practical limit to the number of search keys applied due to performance
 *  and memory requirements.
 *
 *  A key descriptor provides information to generate the search key for one
 *  single key. The search key could be build from several key segments.
 */

// ---- system include files ----
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>

// ---- include files ----
#include <osfio.hpp>
#include <osndxfio.hpp>

// ---- local symbol definitions ----
#define NDXFIO_VERSION  0x01000000 // major.minor.patch - major, minor = 8 bits
#define MAX_MALLOC      (1 << 30)  // maximum memory allocation 2**30

/** Index record status. Do not modify or erase regarding backward
    compatibility! */
enum eINDEX_STATUS
{
    eRESERVED = -2, // Used for file storage.
    eOK       = -1, // Used for file storage.
    eDELETED  = 0   // Index id >= 0.
};

/** Index structure. The index structure is followed by the application key. */
struct sINDEX
{
    union
    {
        S32 status;           // Status of the index record
        S32 prevDeletedIndex; // The offset is not valid when record is deleted.
    };                        // If >= zero this field points to the previous
                              // deleted record.
    U32 offset;               // Byte offset of index record in the file
    U32 dataOffset;           // Byte offset of data record in the file
    U32 dataSize;             // Size of the object in the file, see offset.
    U32 recordRef;            // Verification reference for data records.
    /** KEY **/               // Start of application key

    sINDEX()                  // Constructor.
    :
        status( eRESERVED ),
        offset( U32( INVALID_VALUE )),
        dataOffset( U32( INVALID_VALUE )),
        dataSize( 0 ),
        recordRef( 0 )
    {
    }
};

/** Data record id. Do not modify or erase regarding backward compatibility! */
enum eRECORD_ID
{
    eHEADER       = -4,
    eINDEX        = -3,
    eNEXT_INDEX   = -2,
    eDELETED_DATA = -1, // Deleted data.
    eDATA         = 0   // Data id >= 0.
};

/** Data record struct. Adjacent this record the data is saved. */
struct sDATA
{
    S32 id;                 // Type of data record (eRECORD_ID).
    U32 recordRef;          // Verification reference for data records, should
    union                   // match with record reference given by index record.
    {
        U32 size;           // Number of bytes occupied. Could be less than space
                            // to offset to next record.
        U32 nextIndexOffset;  // Reference to next index record if record id ==
    };                      // NEXT_INDEX.
    U32 offset;             // Offset to next record.
};

/** Database header struct. */
struct sHEADER
{
    U32 version;
    U32 recordReference;    // Verification reference, increased every
                            // record creation.
    U32 nextFreeData;       // Offset to free data position.

    U32 nrOfRecords;        // Number of all valid records (status == eOK).
    U32 nrOfIndexRecords;   // Total of all index records,
                            // status == eOK, eDELETED, eRESERVED.
    S32 lastDeletedIndex;   // Offset to last deleted index record.
    U32 nextFreeIndex;      // Offset to free index position.
    U16 reservedIndexRecords;
    U16 nrOfKeys;           // Number of defined search index keys.
    U16 totalKeySize;       // Sum of all key descriptor segment data
                            // search key sizes. Used for indexing.
    U16 keyDescriptorSize;  // Size sum of all key descriptor segments,
                            // key descriptor is stored adjacent to header.
    sHEADER()               // Constructor.
    :
        version( NDXFIO_VERSION ),
        recordReference( 0 ), // Reference incremented every record creation.
        nextFreeData( 0 ),
        nrOfRecords( 0 ),
        nrOfIndexRecords( 0 ),
        lastDeletedIndex( INVALID_VALUE ),
        nextFreeIndex( 0 ),
        reservedIndexRecords( OSNDXFIO::DEFAULT_RESERVED_INDEX_RECORDS ),
        nrOfKeys( 0 ),
        totalKeySize( 0 ),
        keyDescriptorSize( 0 )
    {
    }
};

/** Key index struct. */
struct sKEY_INDEX
{
    U32* apRecord;
    U32  recordCount;  // ( Record count * sizeof( *apRecord )) == memory
                       // allocated for apRecord.
    U32  position;
    U32  selectionStart;
    U32  selectionEnd;
    U16  keyOffset;
    U16  keySize;
    bool bSorted;

    sKEY_INDEX() // Constructor.
    :
        apRecord( NULL ),
        recordCount( 0 ),
        position( U32( INVALID_VALUE )),
        selectionStart( U32( INVALID_VALUE )),
        selectionEnd( U32( INVALID_VALUE )),
        keyOffset( U16( INVALID_VALUE )),
        keySize( 0 ),
        bSorted( false )
    {
    }
};

/** Database handle list */
struct OSNDXFIO::sHANDLE : sHEADER  // Inherit sHEADER
{
    OSNDXFIO::sHANDLE* pPrevious;
    OSNDXFIO::sHANDLE* pNext;
    OSFIO fileHandle;
    char* pDatabaseName;
    bool readOnly;
    // Key index part.
    bool reindexRequested;
    sKEY_INDEX* apKeyIndex;
    BYTE* apKey; // sINDEX + totalKeySize == totalIndexSize.
    sKEY_DESC* apKeyDescriptor;
    U32 allocatedIndexKeys; // Required for allocating memory for
                            // apKey and apKeyIndex[ keys ].apRecord.
    U16 totalIndexSize;

    sHANDLE() // Constructor.
    :
        pPrevious( NULL ),
        pNext( NULL ),
        fileHandle(),
        pDatabaseName( NULL ),
        readOnly( false ),
        reindexRequested( false ),
        apKeyIndex( NULL ),
        apKey( NULL ),
        apKeyDescriptor( NULL ),
        allocatedIndexKeys( 0 ),
        totalIndexSize( 0 )
    {
    }
};

// ---- local functions prototypes ----
static bool isDatabaseNameValid( const STRING in_databaseName );
static bool isKeyDescriptorValid(
    U16 in_nrOfKeys,
    const OSNDXFIO::sKEY_DESC in_keyDesc[],
    U16& out_keyDescSize,
    U16& out_totalKeySize );
static bool initKeyIndexArray(
    OSNDXFIO::sHANDLE* pHandle,
    U16 key );
static bool initKeyArray( OSNDXFIO::sHANDLE* pHandle );
static bool createReservedIndexRecords(
    OSFIO& handle,
    U32 filePointer,
    U16 reservedIndexRecords,
    U16 totalKeySize );
static void shellSort(
    OSNDXFIO::sHANDLE* const pHandle,
    U16 const in_keyId );
static bool generateSearchKey(
    const OSNDXFIO::sHANDLE* pHandle,
    const OSNDXFIO::sRECORD& in_rRecord,
    BYTE* out_pSearchKey );
static bool convertKeySegment(
    BYTE* pKeySegment,
    OSNDXFIO:: eTYPE in_keySegmentType );
static void swap( U16* pU16 );
static void swap( U32* pU32 );

// ---- local data ----
static OSNDXFIO::sHANDLE* pDatabaseListEntry = NULL;

// ---- constructor ----
OSNDXFIO::OSNDXFIO()
:
    m_handle( NULL ),
    m_error( NO_ERROR )
{
}

// ---- destructor ----
OSNDXFIO::~OSNDXFIO()
{
    if ( NULL != m_handle )
    {
        close();
    }
}

/*============================================================================*/
bool OSNDXFIO::open( const STRING in_databaseName,
                     bool         in_readOnly,
                     U32          in_allocatedIndexKeys )
/*============================================================================*/
{
    m_error = INVALID_PARAMETERS;
    // Check function parameters.
    bool statusOk = isDatabaseNameValid( in_databaseName );

    if ( statusOk )
    {
        m_error  = DATABASE_ALREADY_OPENED;
        statusOk = ( NULL == m_handle );
    }

    if ( statusOk )
    {
        m_handle = new sHANDLE;
        m_error  = MEMORY_ALLOCATION_ERROR;
        statusOk = ( NULL != m_handle );
    }

    // Check database existence.
    if ( statusOk )
    {
        statusOk = m_handle->fileHandle.open( in_databaseName, in_readOnly );
    }

    sDATA data;
    if ( statusOk )
    {
        // Read and verify header.
        m_error  = DATABASE_IO_ERROR;
        statusOk = statusOk && m_handle->fileHandle.read( &data, sizeof( data ));
        // Check record id is correct.
        statusOk = statusOk && ( data.id == eHEADER );
        // Read header.
        statusOk = statusOk && m_handle->fileHandle.read( m_handle, sizeof( sHEADER ));
    }

    if ( statusOk )
    {
        U64 allocatedIndexKeys = m_handle->nrOfIndexRecords + in_allocatedIndexKeys;
        m_error = MEMORY_ALLOCATION_ERROR;
        // Initialize input parameters.
        if ( allocatedIndexKeys < MAX_MALLOC )
        {
            m_handle->readOnly = in_readOnly;
            m_handle->allocatedIndexKeys  = in_readOnly ?
                m_handle->nrOfIndexRecords : (U32)allocatedIndexKeys;
            m_handle->pDatabaseName = (char*)::malloc( ::strlen( in_databaseName ) + 1 );
        }

        statusOk = ( NULL != m_handle->pDatabaseName );
    }

    if ( statusOk )
    {
        ::strcpy( m_handle->pDatabaseName, in_databaseName );

        m_handle->apKeyIndex = (sKEY_INDEX*) ::malloc( m_handle->nrOfKeys * sizeof( sKEY_INDEX ));
        m_handle->apKeyDescriptor = (sKEY_DESC*) ::malloc( m_handle->nrOfKeys * sizeof( sKEY_DESC ));
        // Check if memory allocation was successful.
        statusOk = ( NULL != m_handle->apKeyIndex );
        statusOk = statusOk && ( NULL != m_handle->apKeyDescriptor );
    }

    if ( statusOk )
    {
        U16 totalSegmentSize = 0;
        U16 keyOffset = sizeof( sINDEX );

        // Initialize memory.
        ::memset( m_handle->apKeyIndex, 0, ( m_handle->nrOfKeys * sizeof( sKEY_INDEX )));
        ::memset( m_handle->apKeyDescriptor, 0, ( m_handle->nrOfKeys * sizeof( sKEY_DESC )));

        // Read all key segments and key index records.
        for ( U16 i = 0; statusOk && ( i < m_handle->nrOfKeys ); i++ )
        {
            // Read number of segments.
            statusOk = statusOk && m_handle->fileHandle.read(
                &( m_handle->apKeyDescriptor[ i ].nrOfSegments ),
                sizeof( m_handle->apKeyDescriptor[ 0 ].nrOfSegments ));

            if ( statusOk )
            {
                totalSegmentSize = U16( m_handle->apKeyDescriptor[ i ].nrOfSegments *
                    sizeof( *(m_handle->apKeyDescriptor[ 0 ].apSegment)));
                // Allocate memory for key segments.
                m_handle->apKeyDescriptor[ i ].apSegment = (sKEY_SEGMENT*)
                    ::malloc( totalSegmentSize );
            }

            // Check if memory allocation was successful.
            statusOk = statusOk &&
            ( NULL != m_handle->apKeyDescriptor[ i ].apSegment );

            // Read all segments per key.
            statusOk = statusOk && m_handle->fileHandle.read(
            m_handle->apKeyDescriptor[ i ].apSegment, totalSegmentSize );

            if ( statusOk )
            {
                U16 keySize = 0;

                for ( U16 j = 0; j < m_handle->apKeyDescriptor[ i ].nrOfSegments; j++ )
                {
                    keySize += m_handle->apKeyDescriptor[ i ].apSegment[ j ].size;
                }

                m_handle->apKeyIndex[ i ].keyOffset = keyOffset;
                m_handle->apKeyIndex[ i ].keySize = keySize;

                keyOffset += keySize;

                statusOk = initKeyIndexArray( m_handle, i );
            }
        }
    }

    if ( statusOk )
    {
        U16 keyDescriptorSize = 0;
        U16 totalKeySize      = 0;
        m_error = INVALID_KEY_DESCRIPTOR;
        // Check validity key descriptor.
        statusOk = ( isKeyDescriptorValid( m_handle->nrOfKeys,
            m_handle->apKeyDescriptor, keyDescriptorSize, totalKeySize ) &&
            ( keyDescriptorSize == m_handle->keyDescriptorSize ) &&
            ( totalKeySize == m_handle->totalKeySize ));
    }

    if ( statusOk )
    {
        m_handle->totalIndexSize = (U16)( sizeof( sINDEX ) + m_handle->totalKeySize );

        m_error  = MEMORY_ALLOCATION_ERROR;
        statusOk = initKeyArray( m_handle );

        statusOk = statusOk && m_handle->fileHandle.read( &data, sizeof( data ));
        // Check INDEX has been read.
        statusOk = statusOk && ( data.id == eINDEX );
        // Read all index and application key records.
        BYTE* pByte = (BYTE*)m_handle->apKey;
        U16   reservedIndexCounter = 0;
        for ( U32 k = 0; statusOk && ( k < m_handle->nrOfIndexRecords ); k++ )
        {
            if ( reservedIndexCounter == m_handle->reservedIndexRecords )
            { // Check record ids and read next index offset.
                statusOk = statusOk && m_handle->fileHandle.read( &data, sizeof( data ));
                statusOk = statusOk && ( data.id == eNEXT_INDEX );
                statusOk = statusOk && m_handle->fileHandle.read( data.nextIndexOffset, &data, sizeof( data ));
                statusOk = statusOk && ( data.id == eINDEX );

                reservedIndexCounter = 0;
            }

            // Deleted records are read as well!
            statusOk = statusOk && m_handle->fileHandle.read( pByte, m_handle->totalIndexSize );

            pByte += m_handle->totalIndexSize;
            reservedIndexCounter++;
        }
    }

    if ( statusOk )
    {
        if ( NULL == pDatabaseListEntry )
        {
            pDatabaseListEntry = m_handle;
            pDatabaseListEntry->pPrevious = NULL;
        }
        else
        {
            // Find last NDXFIO handle.
            sHANDLE* pHandle = pDatabaseListEntry;

            while ( NULL != pHandle->pNext )
            {
                pHandle = pHandle->pNext;
            }
            // Update database handle administration.
            pHandle->pNext = m_handle;
            m_handle->pPrevious = pHandle;
        }

        m_handle->pNext = NULL;
        m_error = NO_ERROR;

        for ( U16 keyId = 0; keyId < m_handle->nrOfKeys; keyId++  )
        {
            shellSort( m_handle, keyId );
        }
    }
    else
    {
        close();
    }

    return statusOk;
}

/*============================================================================*/
bool OSNDXFIO::create( const STRING     in_databaseName,
                       U16              in_nrOfKeys,
                       const sKEY_DESC  in_keyDescriptor[],
                       U16              in_reservedIndexRecords )
/*============================================================================*/
{
    // Check function parameters.
    if ( !isDatabaseNameValid( in_databaseName ) ||
       ( 0 == in_nrOfKeys ) ||
       ( NULL == in_keyDescriptor ) ||
       ( in_reservedIndexRecords < 10 ) || // MINIMUM_RESERVED_INDEX_RECORDS
       ( in_reservedIndexRecords > MAXIMUM_RESERVED_INDEX_RECORDS ))
    {
        m_error = INVALID_PARAMETERS;
        UNSUCCESSFUL_RETURN; // Exit create().
    }

    if ( NULL != m_handle )
    {
        m_error = DATABASE_ALREADY_OPENED;
        UNSUCCESSFUL_RETURN; // Exit create().
    }

    U16 keyDescriptorSize = 0;
    U16 totalKeySize = 0;

    if ( !isKeyDescriptorValid(
        in_nrOfKeys,  in_keyDescriptor, keyDescriptorSize,  totalKeySize ))
    {
        m_error = INVALID_KEY_DESCRIPTOR;
        UNSUCCESSFUL_RETURN; // Exit create().
    }

    // Check database existence.
    OSFIO fileHandle;
    if ( fileHandle.open( in_databaseName, READ_ONLY_ACCESS ))
    {
        m_error = DATABASE_ALREADY_EXIST;
        UNSUCCESSFUL_RETURN; // Exit create().
    }

    // Create database.
    bool statusOk = ( fileHandle.create( in_databaseName ));

    if ( statusOk )
    {
        sDATA   record;
        sHEADER header;

        m_error = DATABASE_IO_ERROR;
        /*------------------------------------------------------------------*/
        /*  Data file creation. Since file contents can't be read with      */
        /*  create call alone, file is opened explicitly with open call     */
        /*  after closing the created file                                  */
        /*------------------------------------------------------------------*/
        statusOk = fileHandle.close();

        statusOk = statusOk && fileHandle.open( in_databaseName );

        record.id = eHEADER;
        record.size  = sizeof( header ) + keyDescriptorSize;

        // Data header initialization.
        header.reservedIndexRecords = in_reservedIndexRecords;
        header.nrOfIndexRecords = header.reservedIndexRecords;
        header.nextFreeIndex = sizeof( record /* header id */ ) + record.size +
            sizeof( record /* index id */ );
        header.nextFreeData = header.nextFreeIndex + ( header.reservedIndexRecords *
            ( sizeof( sINDEX ) + totalKeySize )) +
            sizeof( record ); /* next index id */
        header.nrOfKeys = in_nrOfKeys;
        header.totalKeySize = totalKeySize;
        header.keyDescriptorSize = keyDescriptorSize;

        statusOk = statusOk && fileHandle.write( &record, sizeof( record ));
        statusOk = statusOk && fileHandle.write( &header, sizeof( header ));

        for ( int i = 0; statusOk && ( i < in_nrOfKeys ); i++ )
        {
            statusOk = statusOk && fileHandle.write(
                &in_keyDescriptor[ i ].nrOfSegments,
                sizeof( in_keyDescriptor[ 0 ].nrOfSegments ));
            statusOk = statusOk && fileHandle.write(
                in_keyDescriptor[ i ].apSegment, // Pointer to array of key segments.
                ( in_keyDescriptor[ i ].nrOfSegments *
                sizeof( *(in_keyDescriptor[ i ].apSegment))));
        }

        statusOk = statusOk && createReservedIndexRecords(
            fileHandle,
            ( sizeof( record /* header id */ ) +
            record.size ),
            header.reservedIndexRecords,
            header.totalKeySize );
    }

    fileHandle.close();

    if ( statusOk )
    {
        statusOk = open( in_databaseName );
    }

    return statusOk;
}

/*============================================================================*/
bool OSNDXFIO::close()
/*============================================================================*/
{
    if ( NULL != m_handle )
    {
        if ( !m_handle->fileHandle.close() )
        {
            m_error = NO_DATABASE;
        }

        // Update the database administration.
        if ( NULL != m_handle->pPrevious )
        {
            (m_handle->pPrevious)->pNext = m_handle->pNext;
        }

        if ( NULL != m_handle->pNext )
        {
            (m_handle->pNext)->pPrevious = m_handle->pPrevious;
        }

        // Release all allocated memory.
        ::free( m_handle->apKey );

        for ( int i = 0; i < m_handle->nrOfKeys; i++ )
        {
            ::free( m_handle->apKeyDescriptor[ i ].apSegment );
            ::free( m_handle->apKeyIndex[ i ].apRecord );
        }

        ::free( m_handle->apKeyDescriptor );
        ::free( m_handle->apKeyIndex );
        ::free( m_handle->pDatabaseName );

        delete m_handle;
        m_handle = NULL;
    }

    return ( m_error != NO_DATABASE );
}

/*============================================================================*/
bool OSNDXFIO::rebuild( const STRING    in_databaseName,
                        U16             in_nrOfKeys,
                        const sKEY_DESC in_keyDescriptor[],
                        U32             in_maxDataSize )
/*============================================================================*/
{
    U32 nbOfRecords = getNrOfRecords();

    if ( 0 == nbOfRecords )
    {
        m_error = EMPTY_DATABASE;
        UNSUCCESSFUL_RETURN; // Exit rebuild().
    }

    BYTE* pData = (BYTE*)::malloc( in_maxDataSize );

    if ( NULL == pData )
    {
        m_error = MEMORY_ALLOCATION_ERROR;
        UNSUCCESSFUL_RETURN; // Exit rebuild().
    }

    OSNDXFIO rebuild_db;
    bool statusOk = rebuild_db.create( in_databaseName, in_nrOfKeys, in_keyDescriptor,  nbOfRecords );

    sRECORD record;
    record.pData = pData;

    U32 index;
    for ( index = 0; statusOk && ( index < m_handle->nrOfIndexRecords ); index++ )
    {
        sINDEX* pIndex = (sINDEX*)( m_handle->apKey + ( m_handle->totalIndexSize * index ));
        if ( pIndex->status == eOK )
        {
            U32 temp;

            if ( in_maxDataSize < pIndex->dataSize )
            {
                in_maxDataSize       = pIndex->dataSize;
                record.allocatedSize = in_maxDataSize;
                record.pData         = (BYTE*)::realloc( pData, in_maxDataSize );
            }

            statusOk = getRecord( index, record );
            statusOk = statusOk && rebuild_db.createRecord( record, temp );
        }
    }

    ::free( pData );
    (void)rebuild_db.close();

    return statusOk;
}

/*============================================================================*/
U16 OSNDXFIO::getNrOfKeys()
/*============================================================================*/
{
    return m_handle->nrOfKeys;
}

/*============================================================================*/
U16 OSNDXFIO::getKeySize( U16 in_keyId )
/*============================================================================*/
{
    if ( in_keyId < m_handle->nrOfKeys )
    {
        return m_handle->apKeyIndex[ in_keyId ].keySize;
    }

    return 0;
}

/*============================================================================*/
U32 OSNDXFIO::getNrOfRecords()
/*============================================================================*/
{
    return m_handle->nrOfRecords;
}

/*============================================================================*/
bool OSNDXFIO::createRecord( sRECORD& in_rRecord,
                             U32&     out_rIndex )
/*============================================================================*/
{
    BYTE* pSearchKey = (BYTE*)( ::malloc( m_handle->totalKeySize ));

    if ( NULL == pSearchKey )
    {
        m_error = MEMORY_ALLOCATION_ERROR;
        UNSUCCESSFUL_RETURN; // Exit createRecord().
    }

    if ( !generateSearchKey( m_handle, in_rRecord, pSearchKey ))
    {
        m_error = RECORD_TOO_SMALL;
        ::free( pSearchKey );
        UNSUCCESSFUL_RETURN; // Exit createRecord().
    }

    bool deletedRecordAvailable = ( m_handle->lastDeletedIndex >= 0 );
    U32  indexOffset = deletedRecordAvailable ? m_handle->lastDeletedIndex :
                       m_handle->nextFreeIndex;

    sDATA   data;
    sINDEX  index;
    sHEADER header = *m_handle; // Bitwise copy of sHEADER struct of sHANDLE!
    bool    statusOk;

    m_error = DATABASE_IO_ERROR;

    do
    {
        // Read index record.
        statusOk = m_handle->fileHandle.read( indexOffset, &index, sizeof( index ));

        if ( deletedRecordAvailable )
        {
            statusOk = statusOk && ( index.status >= eDELETED );
            // Read data record.
            statusOk = statusOk && m_handle->fileHandle.read( index.dataOffset,
                                                              &data,
                                                              sizeof( data ));
            statusOk = statusOk &&  // Verify data record.
                ( data.id == eDELETED_DATA ) &&
                ( index.recordRef == data.recordRef );

            if ( statusOk )
            {
                if ( in_rRecord.dataSize <= data.size )
                {   // This record is not available anymore for overwriting.
                    deletedRecordAvailable  = false;
                    header.lastDeletedIndex = (U32)index.prevDeletedIndex;
                }
                else
                {
                    indexOffset = index.prevDeletedIndex;

                    if ( indexOffset < 0 ) // There are no more deleted records found.
                    {
                        deletedRecordAvailable = false;
                        // Reread index record and check index record id.
                        statusOk = m_handle->fileHandle.read( m_handle->nextFreeIndex,
                                                              &index,
                                                              sizeof( index ));

                        statusOk = statusOk && ( index.status == eRESERVED );
                    }
                }
            }
        }
        else
        {
            statusOk = statusOk && ( index.status == eRESERVED );
        }
    }
    while ( deletedRecordAvailable );

    if (( statusOk ) && ( index.status == eRESERVED ))
    {
        // Initialize index and data record.
        index.status     = eOK;
        index.dataOffset = m_handle->nextFreeData;
        index.dataSize   = in_rRecord.dataSize;
        index.recordRef  = m_handle->recordReference;

        data.id          = eDATA;
        data.recordRef   = index.recordRef;
        data.size        = in_rRecord.dataSize;
        data.offset      = index.dataOffset + sizeof( data ) + in_rRecord.dataSize;
    }

    if ( statusOk )
    {
        m_error = DATABASE_IO_ERROR;
        // Write data id record.
        statusOk = m_handle->fileHandle.write( index.dataOffset, &data, sizeof( data ));
        // Write data.
        statusOk = statusOk && m_handle->fileHandle.write(( in_rRecord.pData + in_rRecord.dataOffset ), in_rRecord.dataSize );
        // Write index record.
        statusOk = statusOk && m_handle->fileHandle.write( index.offset, &index, sizeof( index ));
        // Write index key.
        statusOk = statusOk && m_handle->fileHandle.write( pSearchKey, header.totalKeySize );
    }

    if ( statusOk )
    {
        // Set record counter and reference.
        header.nrOfRecords++;
        header.recordReference++;
        // Update free data and index.
        header.nextFreeData  += ( sizeof( data ) + in_rRecord.dataSize );

        m_error = DATABASE_IO_ERROR;
        bool reservedIndexRecordsCreated = false;
        // Check for available index records. Reserve index records.
        if ( header.nrOfRecords == m_handle->nrOfIndexRecords )
        {
            // Write extra reserved index records.
            statusOk = createReservedIndexRecords(
                m_handle->fileHandle,
                header.nextFreeData,
                m_handle->reservedIndexRecords,
                m_handle->totalKeySize );

            if ( statusOk )
            {
                // Set the next free index file pointer. Temporary storage.
                header.nextFreeIndex = header.nextFreeData;
                // Update free data file pointer from current file pointer.
                statusOk = (( header.nextFreeData = m_handle->fileHandle.position() ) != (U32)INVALID_VALUE );
                // Calculate the offset for the next index record and retrieve it.
                U32 nextIndexOffset = ( m_handle->nextFreeIndex + sizeof( index ) + header.totalKeySize );
                statusOk = statusOk && m_handle->fileHandle.read( nextIndexOffset, &data, sizeof( data ));
                statusOk = statusOk && ( data.id == eNEXT_INDEX );
                // Set the next index record values.
                data.nextIndexOffset = header.nextFreeIndex;
                data.offset = header.nextFreeIndex;
                // Correction for the eINDEX data record.
                header.nextFreeIndex += sizeof( data );
                // Update the next index record.
                statusOk = statusOk && m_handle->fileHandle.write( nextIndexOffset, &data, sizeof( data ));
                // Set total index record counter.
                header.nrOfIndexRecords += m_handle->reservedIndexRecords;

                reservedIndexRecordsCreated = true;
            }
        }
        else
        {
            header.nextFreeIndex += ( sizeof( index ) + header.totalKeySize );
        }

        // Update file header.
        statusOk = statusOk && m_handle->fileHandle.write( sizeof( data ), &header,
          sizeof( header ));
        // Bitwise copy to first field of sHEADER part of sHANDLE!
        ::memcpy(&m_handle->version, &header, sizeof( header ));

        if ( statusOk && reservedIndexRecordsCreated )
        {
            m_error = MEMORY_ALLOCATION_ERROR;
            // Reinitialize apRecord and apKey array.
            for ( U16 key = 0; statusOk && ( key < m_handle->nrOfKeys ); key++ )
            {
                statusOk = initKeyIndexArray( m_handle, key );
            }

            statusOk = statusOk && initKeyArray( m_handle );
        }
    }

    if ( statusOk )
    {
        U32 prevNrOfRecords = header.nrOfRecords;
        U32 indexOffset     = prevNrOfRecords * m_handle->totalIndexSize;

        // Update apRecord index array and apKey array in memory.
        ::memcpy(( m_handle->apKey + indexOffset), &index, sizeof( index ));
        ::memcpy(( m_handle->apKey + indexOffset + sizeof( index )),
            pSearchKey, m_handle->totalKeySize);

        for ( U16 k = 0; k < m_handle->nrOfKeys; k++ )
        {
            m_handle->apKeyIndex[ k ].bSorted = false;
        }

        m_error     = NO_ERROR;
        out_rIndex  = prevNrOfRecords;
    }

    ::free( pSearchKey );

    return statusOk;
}

/*============================================================================*/
bool OSNDXFIO::getRecord( sKEY&    in_rKey,
                          sRECORD& out_rRecord )
/*============================================================================*/
{
    U32 index = U32( INVALID_VALUE );
    // m_error is set by existRecord() or getRecord( index, .. )
    return ( existRecord( in_rKey, index ) && getRecord( index, out_rRecord ));
}

/*============================================================================*/
bool OSNDXFIO::getRecord( U32      in_index,
                          sRECORD& out_rRecord )
/*============================================================================*/
{
    m_error = DATABASE_IO_ERROR;

    sINDEX* pIndex = (sINDEX*)( m_handle->apKey + ( m_handle->totalIndexSize * in_index ));

    sDATA data;
    // Read data id record.
    bool statusOk = ( m_handle->fileHandle.read( pIndex->dataOffset, &data, sizeof( data )));

    if ( statusOk )
    {
        m_error  = INDEX_CORRUPT;
        // Verify data type and record reference.
        statusOk = (( data.id >= S32( eDATA )) && ( data.recordRef == pIndex->recordRef ));
    }

    if ( statusOk )
    {
        m_error = RECORD_TOO_LARGE;
        // Verify data allocated memory size.
        statusOk = ( data.size <= out_rRecord.allocatedSize);
    }

    if ( statusOk )
    {
        m_error = DATABASE_IO_ERROR;
        // Read data record.
        statusOk = ( m_handle->fileHandle.read(( pIndex->dataOffset +
            sizeof( data )), out_rRecord.pData, data.size ));
    }

    if ( statusOk )
    {
        out_rRecord.dataOffset = pIndex->dataOffset + sizeof( data );
        out_rRecord.dataSize = data.size;
        m_error = NO_ERROR;
    }

    return statusOk;
}

/*============================================================================*/
bool OSNDXFIO::getNextRecord( U16      in_keyId,
                              sRECORD& out_rRecord,
                              U32&     out_rIndex )
/*============================================================================*/
{
    m_error = ENTRY_NOT_FOUND;
    bool statusOk = ( m_handle->apKeyIndex[ in_keyId ].position !=
    m_handle->apKeyIndex[ in_keyId ].selectionEnd );
    out_rIndex = INVALID_VALUE;

    if ( statusOk )
    {
        out_rIndex = m_handle->apKeyIndex[ in_keyId ].
            apRecord[ m_handle->apKeyIndex[ in_keyId ].position ];
        m_handle->apKeyIndex[ in_keyId ].position++;
        statusOk = getRecord( out_rIndex, out_rRecord );
    }

    return statusOk;
}

/*============================================================================*/
bool OSNDXFIO::deleteRecord( U32 in_index )
/*============================================================================*/
{
    m_error        = ENTRY_NOT_FOUND;
    sINDEX* pIndex = (sINDEX*)( m_handle->apKey + ( m_handle->totalIndexSize * in_index ));
    bool  statusOk = ( in_index < m_handle->nrOfIndexRecords );

    statusOk = statusOk && ( eOK == pIndex->status );

    if ( statusOk )
    {
        pIndex->prevDeletedIndex   = m_handle->lastDeletedIndex;
        m_handle->lastDeletedIndex = in_index;
    }

    // TO DO write index + data record!

    return statusOk;
}

/*============================================================================*/
bool OSNDXFIO::updateRecord( U32      in_index,
                             sRECORD& in_rRecord )
/*============================================================================*/
{
    m_error        = NO_ERROR;
    sINDEX* pIndex = (sINDEX*)( m_handle->apKey + ( m_handle->totalIndexSize * in_index ));
    bool  statusOk = ( in_index < m_handle->nrOfIndexRecords );

    sDATA data;
    if ( statusOk )
    {
        m_error = DATABASE_IO_ERROR;
        // Read data id record.
        statusOk = ( m_handle->fileHandle.read( pIndex->dataOffset, &data, sizeof( data )));
    }

    if ( statusOk )
    {
        m_error = INDEX_CORRUPT;
        // Verify data type and record reference.
        statusOk = (( data.id >= S32( eDATA )) && ( data.recordRef == pIndex->recordRef ));
    }

    if ( statusOk )
    {
        m_error = RECORD_TOO_LARGE;
        // Verify available data size.
        statusOk = ( data.offset - ( pIndex->dataOffset + sizeof( data )) >= in_rRecord.dataSize );
    }

    // TO DO write record!

    return statusOk;
}

/*============================================================================*/
bool OSNDXFIO::existRecord( sKEY& in_rKey,
                            U32&  out_rIndex )
/*============================================================================*/
{
    bool bResult = ( m_handle->nrOfRecords > 0 );

    if ( bResult && !in_rKey.conversionDone )
    {
        // m_error is set by convertKey().
        bResult = convertKey( in_rKey );
    }

    if ( bResult )
    {
        if ( !m_handle->apKeyIndex[ in_rKey.id ].bSorted )
        {
          shellSort( m_handle, in_rKey.id );
        }

        m_handle->apKeyIndex[ in_rKey.id ].position       = U32( INVALID_VALUE );
        m_handle->apKeyIndex[ in_rKey.id ].selectionStart = U32( INVALID_VALUE );
        m_handle->apKeyIndex[ in_rKey.id ].selectionEnd   = U32( INVALID_VALUE );
        out_rIndex                                        = U32( INVALID_VALUE );

        if ( m_handle->nrOfRecords == 1 )
        {
            bResult = ( ::memcmp( in_rKey.pValue,
                ( m_handle->apKey + m_handle->apKeyIndex[ in_rKey.id ].keyOffset ),
                in_rKey.size ) == 0 );

            if ( bResult )
            {
                m_handle->apKeyIndex[ in_rKey.id ].position       = 0;
                m_handle->apKeyIndex[ in_rKey.id ].selectionStart = 0;
                m_handle->apKeyIndex[ in_rKey.id ].selectionEnd   = 0;
                out_rIndex                                        = 0;
                in_rKey.index                                     = 0;
                in_rKey.count                                     = 1;
            }
        }
        else
        {
            // Binary search.
            S32 maxIndex   = S32( m_handle->nrOfRecords - 1 );
            S32 leftIndex  = 0;
            S32 rightIndex = maxIndex;
            U32 searchIndex;
            S32 result;

            if ( in_rKey.index != U32( INVALID_VALUE ))
            {
                leftIndex  = in_rKey.index;
                rightIndex = ( in_rKey.index + in_rKey.count );
            }

            do
            {
                searchIndex = U32(( leftIndex + rightIndex ) >> 1 ); // Division by 2.
                result = ::memcmp( in_rKey.pValue,
                    ( m_handle->apKey +
                    ( m_handle->apKeyIndex[ in_rKey.id ].apRecord[ searchIndex ] *
                      m_handle->totalIndexSize ) +
                      m_handle->apKeyIndex[ in_rKey.id ].keyOffset ),
                      in_rKey.size );

                if ( result < 0 )
                {
                    rightIndex = searchIndex - 1;
                }

                if ( result > 0 )
                {
                    leftIndex = searchIndex + 1;
                }

            } while (( result != 0 ) && (leftIndex <= rightIndex));

            if ( result == 0 )
            {
                leftIndex = searchIndex;
                // Find matching keys before searchIndex.
                while (( leftIndex > 0 ) &&
                    ( ::memcmp( in_rKey.pValue,
                    ( m_handle->apKey +
                    ( m_handle->apKeyIndex[ in_rKey.id ].apRecord[ leftIndex - 1 ] *
                      m_handle->totalIndexSize ) +
                      m_handle->apKeyIndex[ in_rKey.id ].keyOffset ),
                    in_rKey.size ) == 0 ))
                {
                    leftIndex--;
                }

                m_handle->apKeyIndex[ in_rKey.id ].position       = U32( leftIndex );
                m_handle->apKeyIndex[ in_rKey.id ].selectionStart = U32( leftIndex );

                rightIndex = searchIndex;
                // Find matching keys beyond searchIndex.
                while (( rightIndex < maxIndex ) &&
                    ( ::memcmp( in_rKey.pValue,
                    ( m_handle->apKey +
                    ( m_handle->apKeyIndex[ in_rKey.id ].apRecord[ rightIndex + 1 ] *
                      m_handle->totalIndexSize ) +
                      m_handle->apKeyIndex[ in_rKey.id ].keyOffset ),
                      in_rKey.size ) == 0 ))
                {
                  rightIndex++;
                }

                m_handle->apKeyIndex[ in_rKey.id ].selectionEnd = U32( rightIndex );

                out_rIndex    = m_handle->apKeyIndex[ in_rKey.id ].apRecord[ leftIndex ];
                in_rKey.index = leftIndex;
                in_rKey.count = U32( rightIndex - leftIndex ) + 1;
            }

            if ( result < 0 )
            {
                bResult       = false;
                m_error       = ENTRY_NOT_FOUND;
                in_rKey.index = searchIndex; // Index for insertion.
            }

            if ( result > 0 )
            {
                bResult       = false;
                m_error       = ENTRY_NOT_FOUND;
                in_rKey.index = searchIndex + 1; // Index for insertion.
            }
        }
    }

    return bResult;
}

/*============================================================================*/
U32 OSNDXFIO::getSearchCount( sKEY& in_rKey )
/*============================================================================*/
{
   return in_rKey.count;
}


/*============================================================================*/
bool OSNDXFIO::convertKey( sKEY& in_rKey )
/*============================================================================*/
{
    m_error = NO_ERROR;
    // Reset conversion done.
    in_rKey.conversionDone = false;

    sKEY_INDEX* pKeyIndex   = &m_handle->apKeyIndex[ in_rKey.id ];
    bool        bResult     = ( in_rKey.size <= pKeyIndex->keySize );
    S32         keySizeLeft = in_rKey.size;

    if ( bResult )
    {
        sKEY_DESC* pKeyDescriptor = &m_handle->apKeyDescriptor[ in_rKey.id ];
        BYTE*      pKey           = in_rKey.pValue;

        for ( U16 j = 0; ( bResult && ( keySizeLeft > 0 ) &&
          ( j < pKeyDescriptor->nrOfSegments )); j++ )
        {
            sKEY_SEGMENT* pKeySegment = &pKeyDescriptor->apSegment[ j ];

            bResult      = convertKeySegment( pKey, pKeySegment->type );
            keySizeLeft -= pKeySegment->size;

            if (( keySizeLeft < 0 ) && ( pKeySegment->type == tBYTE ))
            {
                keySizeLeft = 0;
            }

            pKey += pKeySegment->size;
        }
    }

    bResult = bResult && ( keySizeLeft == 0 );

    if ( bResult )
    {
        in_rKey.conversionDone = true;
    }
    else
    {
        m_error = INVALID_KEY;
    }

    return in_rKey.conversionDone;
}

/*============================================================================*/
OSNDXFIO::eERROR OSNDXFIO::getLastError()
/*============================================================================*/
{
    return m_error;
}

// ---- local functions ----

/*============================================================================*/
static bool isDatabaseNameValid( const STRING in_databaseName )
/*============================================================================*/
{
    return (( NULL != in_databaseName ) && ( "\0" != in_databaseName ));
}

/*============================================================================*/
static bool isKeyDescriptorValid(
                       U16                       in_nrOfKeys,
                       const OSNDXFIO::sKEY_DESC in_keyDesc[],
                       U16&                      out_keyDescSize,
                       U16&                      out_totalKeySize )
/*============================================================================*/
{
    bool bValid = true;

    out_keyDescSize  = 0;
    out_totalKeySize = 0;

    for ( int i = 0; bValid && ( i < in_nrOfKeys ); i++ )
    {
        out_keyDescSize += U16( sizeof( in_keyDesc[ i ].nrOfSegments ));

        for ( int j = 0; bValid && ( j < in_keyDesc[ i ].nrOfSegments ); j++ )
        {
            U16 segmentSize = in_keyDesc[ i ].apSegment[ j ].size;

            // Check given type with size.
            switch ( in_keyDesc[ i ].apSegment[ j ].type )
            {
            case OSNDXFIO::tBYTE:
              bValid = ( segmentSize > 0 );
              break;
            case OSNDXFIO::tS16:
              bValid = ( segmentSize == sizeof( S16 ));
              break;
            case OSNDXFIO::tU16:
              bValid = ( segmentSize == sizeof( U16 ));
              break;
            case OSNDXFIO::tS32:
              bValid = ( segmentSize == sizeof( S32 ));
              break;
            case OSNDXFIO::tU32:
              bValid = ( segmentSize == sizeof( U32 ));
              break;
            default:
              bValid = false;
              break;
            }

            int startKey = in_keyDesc[ i ].apSegment[ j ].offset;
            int stopKey  = in_keyDesc[ i ].apSegment[ j ].size - 1;

            out_keyDescSize  += U16( sizeof( in_keyDesc[ i ].apSegment[ j ] ));
            out_totalKeySize += in_keyDesc[ i ].apSegment[ j ].size;

            stopKey += startKey;

            for ( int k = 0; bValid && ( k < in_keyDesc[ i ].nrOfSegments ); k++ )
            {
                if ( j != k )
                {
                    int startKeyCheck = in_keyDesc[ i ].apSegment[ k ].offset;;
                    int stopKeyCheck  = in_keyDesc[ i ].apSegment[ k ].size - 1;

                    stopKeyCheck += startKeyCheck;

                    bValid = !((( startKey >= startKeyCheck ) && // Inside window
                                ( startKey <= stopKeyCheck )) || // starKey.
                               (( stopKey  >= startKeyCheck ) && // Inside window
                                ( stopKey  <= stopKeyCheck )) || // stopKey.
                               (( startKey <  startKeyCheck)  && // Outside window
                                ( stopKey  >  stopKeyCheck )));  // check.
                }
            }
        }
    }

    if ( !bValid )
    { // Reset.
        out_keyDescSize  = 0;
        out_totalKeySize = 0;
    }

    return bValid;
}

/*============================================================================*/
static bool initKeyIndexArray( OSNDXFIO::sHANDLE* pHandle,
                               U16                key )
/*============================================================================*/
{
    if ( pHandle->allocatedIndexKeys < pHandle->nrOfIndexRecords )
        {
        UNSUCCESSFUL_RETURN; // Exit initKeyIndexArray().
        }

    if ( NULL == pHandle->apKeyIndex[ key ].apRecord )
            {
        pHandle->apKeyIndex[ key ].apRecord = (U32*)::malloc( pHandle->allocatedIndexKeys * sizeof( U32 ));
    }
    // Check if memory allocation was successful.
    bool statusOk = pHandle->apKeyIndex[ key ].apRecord != NULL;

    if ( statusOk )
    {
        U32 i = pHandle->apKeyIndex[ key ].recordCount;
         // Initialize memory.
        for ( ; i < pHandle->nrOfIndexRecords; i++ )
        {
            pHandle->apKeyIndex[ key ].apRecord[ i ] = i;
        }

        // Initialize key count.
        pHandle->apKeyIndex[ key ].recordCount = pHandle->nrOfIndexRecords;
    }

    return statusOk;
}

/*============================================================================*/
static bool initKeyArray( OSNDXFIO::sHANDLE* pHandle )
/*============================================================================*/
{
    U16 totalIndexSize = pHandle->totalIndexSize;
    U64 apKeySize = pHandle->allocatedIndexKeys * totalIndexSize;
    U64 prevApKeySize = pHandle->nrOfRecords * totalIndexSize;
    U64 newApKeySize = pHandle->nrOfIndexRecords * totalIndexSize;

    if ( NULL == pHandle->apKey )
    {
        if ( apKeySize < MAX_MALLOC )
        {
            pHandle->apKey = (BYTE*)::malloc( (U32)apKeySize );
        }
    }
    else if ( newApKeySize > apKeySize )
    {
        if ( newApKeySize < MAX_MALLOC )
        {
            BYTE* apKeyNew = (BYTE*)::realloc( pHandle->apKey, (U32)newApKeySize );

            if (( apKeyNew != NULL ) && ( apKeyNew != pHandle->apKey ))
            {
                ::free( pHandle->apKey );
                pHandle->apKey = apKeyNew;
            }
        }

        pHandle->allocatedIndexKeys = pHandle->nrOfIndexRecords;
    }
    // Check if memory allocation was successful.
    bool statusOk = pHandle->apKey != NULL;

    if ( statusOk )
    {
        if ( newApKeySize > prevApKeySize )
        {
            ::memset(( pHandle->apKey + prevApKeySize ), INVALID_VALUE, ( newApKeySize - prevApKeySize ));
        }
    }

    return statusOk;
}

/*============================================================================*/
static bool createReservedIndexRecords( OSFIO& handle,
                                        U32    filePointer,
                                        U16    reservedIndexRecords,
                                        U16    totalKeySize )
/*============================================================================*/
{
    void* pKey           = malloc( totalKeySize );
    bool  statusOk       = ( NULL != pKey );
    sDATA record;
    U32   indexOffset    = filePointer + sizeof( record /* index id */);
    U32   totalIndexSize = sizeof( sINDEX ) + totalKeySize;

    if ( statusOk )
    {
        ::memset( pKey, 0, totalKeySize ); // Initialize search key.

        record.id     = eINDEX;
        record.size   = reservedIndexRecords * totalIndexSize;
        record.offset = indexOffset + record.size;
    }

    // Write data record with index id.
    statusOk = statusOk && handle.write( filePointer,
                                       &record,
                                       sizeof( record ));
    sINDEX index;

    for ( U16 j = 0; statusOk && ( j < reservedIndexRecords ); j++ )
    {
        index.offset = indexOffset; // Write the index record.
        statusOk     = statusOk && handle.write(&index, sizeof( index ));
        statusOk     = statusOk && handle.write( pKey, totalKeySize );

        indexOffset += totalIndexSize;
    }

    ::free( pKey );

    record.id              = eNEXT_INDEX;
    record.nextIndexOffset = 0; // Initialize fields, there is no next
    record.offset          = 0; // index block of reserved index yet.

    // Write data record with next index id.
    statusOk  = statusOk && handle.write( &record, sizeof( record ));

    return statusOk;
}

/*============================================================================*/
static void shellSort( OSNDXFIO::sHANDLE* const pHandle,
                       U16 const in_keyId  )
/*============================================================================*/
{
    U32 incrementalGap = 1;
    U32 nrOfRecords = pHandle->nrOfRecords;

    /*--------------------------------------------------------------*/
    /* Compute the initial increment gap. The best algorithm is     */
    /* unknown. Knuth's recommendation to compute the inc(k) is:    */
    /* inc(1) = 1; inc(k+1) = 3*inc(k)+1; stop if inc(k+2) >= nelem */
    /*--------------------------------------------------------------*/
    if ( nrOfRecords <= 13 )
    {
        incrementalGap = 1; // Straight insertion sort.
    }
    else
    {
        do
        {
            incrementalGap *= 3; // No check for potential overflow!
            incrementalGap += 1;
        }
        while ( incrementalGap < nrOfRecords );

        incrementalGap /= 3;
        incrementalGap /= 3;
    }

    BYTE* apKey = pHandle->apKey;
    U16 totalIndexSize = pHandle->totalIndexSize;

    /*--------------------------------------------------------------*/
    /* Perform (diminishing increment) Shell Sort. Skip the inner   */
    /* inner loop and redundant swap if elem_i is already sorted.   */
    /*--------------------------------------------------------------*/
    do
    {
        for ( U32 i = incrementalGap; i < nrOfRecords; i++ )
        {
            U32  keyOffset      = pHandle->apKeyIndex[ in_keyId ].keyOffset;
            U32  keySize        = pHandle->apKeyIndex[ in_keyId ].keySize;
            U32  j = i;
            U32  indexI         = pHandle->apKeyIndex[ in_keyId ].apRecord[ i ];
            U32  indexJMinusInc = pHandle->apKeyIndex[ in_keyId ].apRecord[ j - incrementalGap ];
            U32* pIndexJ        = &pHandle->apKeyIndex[ in_keyId ].apRecord[ j ];

            if ( ::memcmp(( apKey + ( indexJMinusInc * totalIndexSize ) + keyOffset ),
            ( apKey + ( indexI * totalIndexSize ) + keyOffset ),
            keySize ) > 0 )
            {
                bool bMinusInc;
                do
                {
                    *pIndexJ = indexJMinusInc;

                    j -= incrementalGap;

                    bMinusInc = ( j >= incrementalGap );
                    pIndexJ = &pHandle->apKeyIndex[ in_keyId ].apRecord[ j ];

                    if ( bMinusInc )
                    {
                        indexJMinusInc = pHandle->apKeyIndex[ in_keyId ].apRecord[ j - incrementalGap ];
                    }
                }
                while ( bMinusInc &&
                  ( ::memcmp(( apKey + ( indexJMinusInc * totalIndexSize ) + keyOffset ),
                    ( apKey + ( indexI * totalIndexSize ) + keyOffset ),
                    keySize ) > 0 ));

                *pIndexJ = indexI;
            }
        }

        incrementalGap /= 3;

    } while ( incrementalGap > 0 );

    pHandle->apKeyIndex[ in_keyId ].bSorted = true;
}

/*============================================================================*/
static bool generateSearchKey( const OSNDXFIO::sHANDLE* pHandle,
                               const OSNDXFIO::sRECORD& in_rRecord,
                               BYTE*                    out_pSearchKey )
/*============================================================================*/
{
    bool bResult = true;

    for ( U16 i = 0; bResult && ( i < pHandle->nrOfKeys ); i++ )
    {
        OSNDXFIO::sKEY_DESC* pKeyDescriptor = &pHandle->apKeyDescriptor[ i ];

        for ( U16 j = 0; bResult && ( j < pKeyDescriptor->nrOfSegments ); j++ )
        {
            OSNDXFIO::sKEY_SEGMENT* pKeySegment = &pKeyDescriptor->apSegment[ j ];

            bResult = (( U32(pKeySegment->offset) + U32(pKeySegment->size)) <=
                ( in_rRecord.dataOffset + in_rRecord.dataSize ));

            if ( bResult )
            { // Copy the search key segment.
                ::memcpy( out_pSearchKey, ( in_rRecord.pData + pKeySegment->offset ),
                    pKeySegment->size );

                bResult = convertKeySegment( out_pSearchKey, pKeySegment->type );
            }

            if ( bResult )
            {
                out_pSearchKey += pKeySegment->size;
            }
        }
    }

    return bResult;
}

/*============================================================================*/
static bool convertKeySegment( BYTE*            pKeySegment,
                               OSNDXFIO:: eTYPE in_keySegmentType)
/*============================================================================*/
{
    bool bResult = true;
    U16* pU16;
    U32* pU32;

    switch ( in_keySegmentType )
    {
    case OSNDXFIO::tBYTE:
        // tBYTE, do nothing.
        break;
    case OSNDXFIO::tS16:
        pU16 = (U16*)pKeySegment;
        *pU16 += U16(0x8000);     // Signed correction.
        #ifndef CPU_BIG_ENDIAN    // default CPU_LITTLE_ENDIAN!
        swap( pU16 );
        #endif
        break;
    case OSNDXFIO::tU16:
        pU16 = (U16*)pKeySegment;
        #ifndef CPU_BIG_ENDIAN    // default CPU_LITTLE_ENDIAN!
        swap( pU16 );
        #endif
        break;
    case OSNDXFIO::tS32:
        pU32 = (U32*)pKeySegment;
        *pU32 += U32(0x80000000); // Signed correction.
        #ifndef CPU_BIG_ENDIAN    // default CPU_LITTLE_ENDIAN!
        swap( pU32 );
        #endif
        break;
    case OSNDXFIO::tU32:
        pU32 = (U32*)pKeySegment;
        #ifndef CPU_BIG_ENDIAN    // default CPU_LITTLE_ENDIAN!
        swap( pU32 );
        #endif
        break;
    default:
        bResult = false;
        break;
    }

    return bResult;
}

/*============================================================================*/
static void swap( U16* pU16 )
/*============================================================================*/
{
    BYTE copy = *(BYTE*)pU16;
    ::memcpy( pU16, ( (BYTE*)pU16 + 1 ), 1 );
    ::memcpy(( (BYTE*)pU16 + 1 ), &copy, 1 );
}

/*============================================================================*/
static void swap( U32* pU32 )
/*============================================================================*/
{
    BYTE copy = *(BYTE*)pU32;
    ::memcpy( pU32, ( (BYTE*)pU32 + 3 ), 1 );
    ::memcpy(( (BYTE*)pU32 + 3 ), &copy, 1 );
    copy = *( (BYTE*)pU32 + 1 );
    ::memcpy(( (BYTE*)pU32 + 1 ), ( (BYTE*)pU32 + 2 ), 1 );
    ::memcpy(( (BYTE*)pU32 + 2 ), &copy, 1 );
}

