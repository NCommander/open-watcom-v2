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
* Description:  Strings data
*
****************************************************************************/


#include "wipfc.hpp"
#include <cstdlib>
#include "strings.hpp"
#include "errors.hpp"
#include "document.hpp"
#include "util.hpp"


STD1::uint32_t StringTable::write( std::FILE *out, Document *document )
{
    if( _table.empty() )
        return 0L;
    STD1::uint32_t start( std::ftell( out ) );
    for( ConstTableIter itr = _table.begin(); itr != _table.end(); ++itr ) {
        char buffer[ 256 ];     // max len 255 + null
        std::size_t written;
        std::size_t length( document->wtomb_cstring( buffer, itr->c_str(), sizeof( buffer ) - 1 ) );
        if( length == ERROR_CNV )
            throw FatalError( ERR_T_CONV );
        if( std::fputc( static_cast< STD1::uint8_t >( length + 1 ), out ) == EOF ||
            ( written = std::fwrite( buffer, sizeof( char ), length, out ) ) != length)
            throw FatalError( ERR_WRITE );
        _bytes += written + 1;
    }
    return start;
}

