#ifndef OSNDXFIO_HPP
#define OSNDXFIO_HPP
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

// ---- include files ----
#include <osdef.h>

class OSNDXFIO
{
public:
// Error codes.
enum eERROR
{
    DATABASE_ALREADY_EXIST,
    DATABASE_ALREADY_OPENED,
    DATABASE_IO_ERROR,
    EMPTY_DATABASE,
    ENTRY_NOT_FOUND,
    INDEX_CORRUPT,
    INVALID_DATABASE,
    INVALID_INDEX,
    INVALID_KEY,
    INVALID_KEY_DESCRIPTOR,
    INVALID_PARAMETERS,
    INVALID_KEY_INDEX,
    MEMORY_ALLOCATION_ERROR,
    NO_DATABASE,
    NO_ERROR,
    NO_RECORD,
    RECORD_TOO_LARGE,
    RECORD_TOO_SMALL,
    SIZE_MISMATCH,
    TOO_MANY_RECORDS,
};

/** Type definitions used for building index keys. Do not modify or erase
  regarding backward compatibility! */
enum eTYPE
{
    tBYTE = 1,
    tS16  = 2,
    tU16  = 3,
    tS32  = 4,
    tU32  = 5
};

enum
{
    MINIMUM_RESERVED_INDEX_RECORDS = 10,    // Minimum and maximum values are checked,
    DEFAULT_RESERVED_INDEX_RECORDS = 100,   // optimum depends on application
    MAXIMUM_RESERVED_INDEX_RECORDS = 10000, // U16 in_reservedIndexRecords
    DEFAULT_ALLOCATED_INDEX_KEYS = 50000,   // U32 in_allocatedIndexKeys
    MAXIMUM_DATA_SIZE = 1000                // U32 in_maxDataSize
};

/**
*  The search key segment structure. A key descriptor, consisting of multiple
*  key (type) segments is applied on every data record.
*
*  Data record pointer ----------------------------------------------------->
*  ----->offset1, size1 -------->offset2, size2 ----------->offset3, size3
*  e.g...|----tByte----|.........|--tS16--|.................|-tByte-|........
*
*  The key description should fall within all data record sizes plus there
*  offsets. The key build from the segment is copied into the database. If
*  the key is based on the part before the data offset it is only stored in
*  as an index and is not copied from the actual data. If data records are
*  small but have multiple search keys the storage space occupied by the
*  search keys could be more than the actual data. The search keys are also
*  copied into memory for fast search, so this takes memory space as well.
*
*  Key searches are memory based therefore it is important to know wether the
*  machine running is a LITTLE or BIG ENDIAN machine. Default a little endian
*  machine is expected, #define CPU_BIG_ENDIAN for a big endian machine.
*/
struct sKEY_SEGMENT
{
    U16  offset; // The offset of key segment.
    BYTE type;   // The type of the key segment, required for key matching.
    BYTE size;   // The size of key segment.

    sKEY_SEGMENT() // Constructor.
    :
        offset( U16( INVALID_VALUE )),
        type( BYTE( tBYTE )),
        size( 0 )
    {
    }

    sKEY_SEGMENT( U16 in_offset, // Constructor.
                  eTYPE in_type,
                  U16 in_size )
    :
        offset( in_offset ),
        type( BYTE( in_type )),
        size( in_size )
    {
    }
};

/** Application key descriptor structure. */
struct sKEY_DESC
{
    U16           nrOfSegments;
    sKEY_SEGMENT* apSegment; // Pointer to array of key segments.
};

/** Application key structure. */
struct sKEY
{
    friend class OSNDXFIO;

    U16   id;      // The search key index.
    U16   size;    // Length of search key.
    BYTE* pValue;  // The search key.

    sKEY()         // Constructor.
    :
        id( U16( INVALID_VALUE )),
        size( 0 ),
        pValue( NULL ),
        conversionDone( false ),
        index( U32( INVALID_VALUE )),
        count( 0 )
    {
    }
                   // Constructor.
    sKEY( U16   in_id,
          U16   in_size,
          BYTE* in_pValue )
    :
        id( in_id ),
        size( in_size ),
        pValue( in_pValue ),
        conversionDone( false ),
        index( U32( INVALID_VALUE )),
        count( 0 )
    {
    }

    private:
    bool conversionDone;
                   // Conversion key required for signed key segment types and
                   // little endian numbers. Default false. Set true by
                   // convertKey().
    U32  index;    // Key index identification of the first record found. Set by
                   // getRecord() and existRecord(). This index is internally
                   // used and not the same index as retrieved by existRecord().
    U32  count;    // Number of records found matching the search key. Set by
                   // getRecord() and existRecord().
};

/** Application object structure. */
struct sRECORD
{
    U32   allocatedSize;  // Allocated size.
    U32   dataOffset;     // The offset from where the actual data is stored.
    U32   dataSize;       // Actual size.
    BYTE* pData;          // Points to actual data.


    sRECORD()             // Constructor.
    :
        allocatedSize( 0 ),
        dataOffset( 0 ),
        dataSize( 0 ),
        pData( NULL )
    {
    }

    sRECORD( U32   in_allocatedSize, // Constructor.
             U32   in_dataOffset,
             U32   in_dataSize,
             BYTE* in_pData )
    :
        allocatedSize( in_allocatedSize ),
        dataOffset( in_dataOffset ),
        dataSize( in_dataSize ),
        pData( in_pData )
    {
    }
};

OSNDXFIO();  // Constructor.
~OSNDXFIO(); // Destructor.

// Database level functions

/**
*  Opens an existing database.
*
*  @param  in_databaseName   File name of the database.
*  @param  in_readOnly       Flag to indicate readonly access.
*  @param  in_allocatedIndexKeys
*                            Number of index keys to be allocated in memory (in
                             advance).
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*  @post   OSNDXFIO operations can be performed on the database if successful.
*/
bool open( const STRING in_databaseName,
             bool         in_readOnly = false,
             U32          in_allocatedIndexKeys = DEFAULT_ALLOCATED_INDEX_KEYS );

/**
*  Creates and opens a new indexed database.
*
*  @param  in_databaseName   File name of the database.
*  @param  in_nrOfKeys       Number of index keys.
*  @param  in_keyDescriptor  Description (array) of every index key.
*  @param  in_reservedIndexRecords
*                            Number of index record to be reserved (in case
*                            of creation of records).
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*  @post   OSNDXFIO operations can be performed on the created database.
*/
bool create( const STRING    in_databaseName,
             U16             in_nrOfKeys,
             const sKEY_DESC in_keyDescriptor[],
             U16             in_reservedIndexRecords = DEFAULT_RESERVED_INDEX_RECORDS );

/**
*  Closes a previously opened indexed database. The indexed database is also
*  automatically closed when the OSNDXFIO object is destroyed.
*
*  @pre    Opened indexed database.
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*/
bool close();

/**
*  Rebuilds an existing indexed database with another key descriptor.
*
*  Warning: Take care if the existing key index is based on data before the
*  data offset.
*
*  @pre    Opened indexed database.
*  @param  in_databaseName   File name of the rebuild database.
*  @param  in_nrOfKeys       Number of index keys.
*  @param  in_keyDescriptor  Description (array) of every index key.
*  @param  in_maxDataSize    Mazimum expected data size (for memory allocation).
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*/
bool rebuild( const STRING    in_databaseName,
              U16             in_nrOfKeys,
              const sKEY_DESC in_keyDescriptor[],
              U32             in_maxDataSize = MAXIMUM_DATA_SIZE );

/** Returns number of keys of open database. */
U16 getNrOfKeys();

/**
*  Returns key size of key index of open database.
*
*  @param  in_keyId      The key index (0 - (numberOfKeys - 1)).
*  @return 0             If in_keyId does not exist.
*/
U16 getKeySize( U16 in_keyId );

/** Returns number of records of open database. */
U32 getNrOfRecords();

// Data object functions

/**
*  Creates a data record.
*
*  @pre    Opened indexed database.
*  @param  in_rRecord  Data record what includes pointer to actual data.
*                      Struct field out_rRecord.dataSize gives actual
*                      size. Struct field out_rRecord.allocatedSize is not
*                      relevant and is ignored.
*  @param  out_rIndex  Index identification of created record.
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*/
bool createRecord( sRECORD& in_rRecord,
                   U32&     out_rIndex );

/**
*  Retrieves a key based data record.
*
*  @pre    Opened indexed database. Amount out_rRecord.pData memory
*          allocation for retrieved data record given by
*          out_rRecord.allocatedSize.
*  @param  in_rKey       Search key what includes pointer to actual key.
*                        Partial key search is allowed. The first record
*                        matching the (partial) key is returned. Use
*                        getNextRecord() to retrieve the rest.
*  @param  out_rRecord   Data record what includes pointer to actual data.
*                        Struct field out_rRecord.dataSize returns actual
*                        size and should be equal or less than struct
*                        field out_rRecord.allocatedSize.
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*/
bool getRecord( sKEY&    in_rKey,
                sRECORD& out_rRecord );

/**
*  Retrieves a index based data record.
*
*  @pre    Opened indexed database. Amount out_rRecord.pData memory
*          allocation for retrieved data record given by
*          out_rRecord.allocatedSize.
*  @param  in_index      Index identification of specific record.
*  @param  out_rRecord   Data record what includes pointer to actual data.
*                        Struct field out_rRecord.dataSize returns actual
*                        size.
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*/
bool getRecord( U32      in_index,
                sRECORD& out_rRecord );

/**
*  Retrieves the next data record after getRecord() based on search key.
*
*  @pre    Opened indexed database. Amount out_rRecord.pData memory
*          allocation for retrieved data record given by
*          out_rRecord.allocatedSize.
*  @param  in_keyId      The key index (0 - (numberOfKeys - 1).
*  @param  out_rRecord   Data record what includes pointer to actual data.
*                        Struct field out_rRecord.size returns actual size.
*  @param  out_rIndex    Index identification of next record.
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*/
bool getNextRecord( U16      in_keyId,
                    sRECORD& out_rRecord,
                    U32&     out_rIndex );

/**
*  Deletes a data record.
*
*  @pre    Opened indexed database.
*  @param  in_index    Index identification of specific record.
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*/
bool deleteRecord( U32 in_index );

/**
*  Updates a data record.
*
*  @pre    Opened indexed database.
*  @param  in_index      Index identification of specific record.
*  @param  in_rRecord    Data record what includes pointer to actual data.
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*/
bool updateRecord( U32      in_index,
                   sRECORD& in_rRecord );

/**
*  Checks for existing data record. The same sKey could be reused for search
*  after modifying the key value and size.
*
*  @pre    Opened indexed database.
*  @param  in_rKey       Search key what includes pointer to actual key.
*                        Partial key search is allowed. The first record
*                        matching the (partial) key is returned. Use
*                        getNextIndex() to retrieve the rest.
*  @param  out_rIndex    Index identification of the first found record.
*  @return True if successful. On false error could be retrieved with
*          getLastError() == ENTRY_NOT_FOUND.
*/
bool existRecord( sKEY& in_rKey,
                  U32&  out_rIndex );

/**
*  Gets search key count after getRecord() or existRecord().
*
*  @pre    Opened indexed database and getRecord or existRecord().
*  @param  in_rKey      Search key what includes pointer to actual key.
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*/
U32 getSearchCount( sKEY& in_rKey );

/**
*  Retrieves the next index identification from a existRecord() request.
*  Could be used in combination with index based getRecord().
*
*  @pre    Opened indexed database.
*  @param  in_keyId      The key index (0 - (numberOfKeys - 1)).
*  @param  out_rIndex    Index identification of the next record found.
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*/
bool getNextIndex( U16  in_keyId,
                   U32& out_rIndex );

/**
*  Convert search key. Required for signed key segment types and little
*  endian numbers.
*
*  @pre    Opened indexed database.
*  @param  in_rKey           Search key what includes pointer to actual key.
*                            Partial key conversion is allowed.
*  @return True if successful. On false error could be retrieved with
*          getLastError().
*/
bool convertKey( sKEY& in_rKey );

/**
*  Retrieves the last error generated.
*
*  @pre    Opened indexed database.
*  @note   Last error will be reset to NO_ERROR on entering method which
*          could set the last error.
*  @return eERROR
*/
eERROR getLastError();

// Forward declaration.
struct sHANDLE;

private:
sHANDLE* m_handle;
eERROR   m_error;

// Copy construction and assignment are prevented.
OSNDXFIO( const OSNDXFIO& );
OSNDXFIO& operator=( OSNDXFIO& );
};
#endif // OSNDXFIO_HPP

