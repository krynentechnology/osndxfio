#ifndef OSFIO_HPP
#define OSFIO_HPP
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
 *  Description: File I/O
 */

// ---- include files ----
#include <osdef.h>

// ---- symbol definitions ----
#define READ_ONLY_ACCESS  true
#define READ_WRITE_ACCESS false
#define EOF_POSITION      U32(-1)

class OSFIO
{
public:

OSFIO();
~OSFIO();

/**
*  Opens an existing file.
*
*  @param    in_fileName   File name of the file.
*  @param    in_readOnly   Flag to indicate readonly access.
*  @return   true if successful.
*  @post     FIO operations can be performed on the opened file.
*/
bool open( const STRING in_fileName,
           bool         in_readOnly = false );

/**
*  Creates a new file (RW) if the file does not exist.
*
*  @param    in_fileName   File name of the file.
*  @return   true if successful.
*  @post     FIO operations can be performed on the created file.
*/
bool create( const STRING in_fileName );

/**
*  Closes a previously opened file. The file is also automatically closed
*  when the OSFIO object is destroyed.
*
*  @pre      Opened indexed database.
*  @return   true if successful.
*/
bool close();

/**
*  Deletes an existing file even if it is read-only.
*
*  @param    in_fileName   File name of the file.
*  @return   true if successful.
*/
static bool erase( const STRING in_fileName );

/**
*  Writes data to an opened or created file.
*
*  @pre      Valid handle by open() or create().
*  @param    in_dataPtr    The pointer to data.
*  @param    in_dataSize   The number of bytes transfered. Default 1 byte.
*  @return   true if successful.
*/
bool write( const POINTER in_dataPtr,
            U32           in_dataSize = 1 );

/**
*  Writes data to an opened or created file.
*
*  @pre      Valid handle by open() or create().
*  @param    in_position   The byte offset from the beginning of the file.
*                        If EOF_POSITION the data is appended.
*  @param    in_dataPtr    The pointer to data.
*  @param    in_dataSize   The number of bytes transfered. Default 1 byte.
*  @return   true if successful.
*/
bool write( U32           in_position,
            const POINTER in_dataPtr,
            U32           in_dataSize = 1 );

/**
*  Reads data from an opened file.
*
*  @pre      Valid handle by open() or create().
*  @param    out_dataPtr   The pointer to data.
*  @param    in_dataSize   The number of bytes transfered. Default 1 byte.
*  @return   true if successful.
*/
bool read( POINTER out_dataPtr,
           U32     in_dataSize = 1 );

/**
*  Reads data from an opened file.
*
*  @pre      Valid handle by open() or create().
*  @param    in_position   The byte offset from the beginning of the file.
*  @param    out_dataPtr   The pointer to data.
*  @param    in_dataSize   The number of bytes transfered. Default 1 byte.
*  @return   true if successful.
*/
bool read( U32     in_position,
           POINTER out_dataPtr,
           U32     in_dataSize = 1 );

/**
*  Indicates the end of files status of the file pointer position.
*
*  @pre      Valid handle by open() or create().
*  @return   true if successful.
*/
bool eof();

/**
*  Gives the size of file.
*
*  @pre      Valid handle by open() or create().
*  @return   (U32)INVALID_VALUE on failure.
*/
U32 size();

/**
*  Gives the file pointer position. Byte offset from start of file.
*
*  @pre      Valid handle by open() or create().
*  @return   (U32)INVALID_VALUE on failure.
*/
U32 position();

/**
*  Truncates the file at given file pointer position.
*
*  @pre      Valid handle by open() or create().
*  @param    in_position   The byte offset from the start of the file.
*  @return   true if successful.
*/
bool truncate( U32 in_position );

/**
*  Returns time of last modification in seconds since midnight (00:00:00),
*  1 January 1970.
*
*  @pre      Valid handle by open() or create().
*  @return   (U32)INVALID_VALUE on failure.
*/
U32 timestamp();

private:
int m_handle;
};
#endif  // OSFIO_HPP

