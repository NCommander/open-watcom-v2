/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2009-2018 The Open Watcom Contributors. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Panel Controls data
*
****************************************************************************/


#include "wipfc.hpp"
#include "ctrlbtn.hpp"
#include "errors.hpp"
#include "outfile.hpp"


ControlButton::dword ControlButton::write( OutFile *out ) const
{
    std::size_t bytes( sizeof( word ) * 2 );

    // type = 1
    if( out->put( static_cast< word >( 1 ) ) )
        throw FatalError( ERR_WRITE );
    if( out->put( _res ) )
        throw FatalError( ERR_WRITE );
    std::string buffer;
    out->wtomb_string( _txt, buffer );
    std::size_t length( buffer.size() );
    if( length > 255 ) {
        buffer.erase( 255 );
        length = 255;
    }
    if( out->put( static_cast< byte >( length ) ) )
        throw FatalError( ERR_WRITE );
    if( out->write( buffer.data(), sizeof( char ), length ) )
        throw FatalError( ERR_WRITE );
    bytes += length + 1;
    return( static_cast< dword >( bytes ) );
}
