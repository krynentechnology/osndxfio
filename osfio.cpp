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
#include <stdio.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <sys\types.h>
#include <io.h>

// ---- application includes ----
#include <osfio.hpp>

// ---- local symbol definitions ----
#define SUCCESSFUL        0
#define ERROR             (-1)

// ---- constructor ----
OSFIO::OSFIO()
:
    m_handle( NULL )
{
}

// ---- destructor ----
OSFIO::~OSFIO()
{
    if ( m_handle != NULL )
    {
        close();
    }
}

/*============================================================================*/
bool OSFIO::open( const STRING in_fileName,
                  bool         in_readOnly )
/*============================================================================*/
{
    if ( m_handle != NULL )
    {
        return false;
    }

    int handle = ::open( in_fileName, (( in_readOnly ? O_RDONLY : O_RDWR ) | O_BINARY ));

    bool result = ( handle != ERROR );

    if ( result )
    {
        m_handle = (HANDLE)handle;
    }

    return result;
}

/*============================================================================*/
bool OSFIO::create( const STRING in_fileName )
/*============================================================================*/
{
    if ( m_handle != NULL )
    {
        return false;
    }

    if ( open( in_fileName, READ_ONLY_ACCESS ))
    {
        close();
        return false;
    }
    else
    {
        m_handle = (HANDLE)::open( in_fileName, ( O_CREAT | O_BINARY ), ( S_IWRITE | S_IREAD ));
    }

    return ( int( m_handle ) != ERROR );
}

/*============================================================================*/
bool OSFIO::close()
/*============================================================================*/
{
    if ( m_handle == NULL )
    {
        return false;
    }

    bool status_ok = ( ::close( int( m_handle )) != ERROR );

    m_handle = NULL;

    return status_ok;
}

/*============================================================================*/
bool OSFIO::erase( const STRING in_fileName )
/*============================================================================*/
{
    // Allow to delete. Might be read-only.
    ::chmod( in_fileName, ( S_IWRITE | S_IREAD ));

    return ( ::remove( in_fileName ) == SUCCESSFUL );
}

/*============================================================================*/
bool OSFIO::write( const POINTER in_dataPtr,
                   U32           in_dataSize )
/*============================================================================*/
{
    if ( m_handle == NULL )
    {
        return false;
    }

    return ( ::write( int( m_handle ), in_dataPtr, in_dataSize ) != ERROR );
}

/*============================================================================*/
bool OSFIO::write( U32           in_position,
                   const POINTER in_dataPtr,
                   U32           in_dataSize )
/*============================================================================*/
{
    if ( m_handle == NULL )
    {
        return false;
    }

    return (( ::lseek( int( m_handle ),
        (( in_position == EOF_POSITION ) ? 0 : in_position ),
        (( in_position == EOF_POSITION ) ? SEEK_END : SEEK_SET )) != ERROR ) &&
        ( ::write( int( m_handle ), in_dataPtr, in_dataSize ) != ERROR ));
}

/*============================================================================*/
bool OSFIO::read( POINTER out_dataPtr,
                  U32     in_dataSize )
/*============================================================================*/
{
    if ( m_handle == NULL )
    {
        return false;
    }

    return ( (U32)::read( int( m_handle ), out_dataPtr, in_dataSize ) ==
        in_dataSize );
}

/*============================================================================*/
bool OSFIO::read( U32     in_position,
                  POINTER out_dataPtr,
                  U32     in_dataSize )
/*============================================================================*/
{
    if ( m_handle == NULL )
    {
        return false;
    }

    return (( ::lseek( int( m_handle ), in_position, SEEK_SET ) != ERROR ) &&
        ( (U32)::read( int( m_handle ), out_dataPtr, in_dataSize ) ==
        in_dataSize ));
}

/*============================================================================*/
bool OSFIO::eof()
/*============================================================================*/
{
    if ( m_handle == NULL )
    {
        return false;
    }

    return ( ::eof( int( m_handle ) ) == 1 );
}

/*============================================================================*/
U32 OSFIO::size()
/*============================================================================*/
{
    return (U32)::filelength( int( m_handle ));
}

/*============================================================================*/
U32 OSFIO::position()
/*============================================================================*/
{
    return (U32)::tell( int( m_handle ));
}

/*============================================================================*/
bool OSFIO::truncate( U32 in_position )
/*============================================================================*/
{
    if ( m_handle == NULL )
    {
        return false;
    }

    U32  fileSize  = size();
    bool status_ok = ( fileSize != (U32)INVALID_VALUE );

    if ( status_ok )
    {
        status_ok = ( in_position < fileSize );
        status_ok = status_ok && ( ::chsize( int( m_handle ), in_position ) != ERROR );
        // set file pointer correct
        status_ok = status_ok && ( ::lseek( int( m_handle ), 0, SEEK_END ) != ERROR );
    }

    return status_ok;
}

/*============================================================================*/
U32 OSFIO::timestamp()
/*============================================================================*/
{
    struct stat statBuffer;

    if ( ::fstat( int( m_handle ), &statBuffer ) == 0 )
    {
        return (U32)statBuffer.st_mtime;
    }

    return (U32)INVALID_VALUE;
}

