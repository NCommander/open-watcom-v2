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
* Description:  An index entry
*
****************************************************************************/


#include "wipfc.hpp"
#include <cwctype>
#include "index.hpp"
#include "errors.hpp"
#include "outfile.hpp"


IndexItem::IndexItem( Type t )
{
    if( t == PRIMARY ) {
        _hdr.primary = 1;
    } else if( t == SECONDARY ) {
        _hdr.secondary = 1;
    }
}
/***************************************************************************/
bool IndexItem::operator==( const IndexItem& rhs ) const
{
    if( _sortKey.empty() ) {
        if( rhs._sortKey.empty() ) {
            return wstricmp( _text.c_str(), rhs._text.c_str() ) == 0;
        } else {
            return wstricmp( _text.c_str(), rhs._sortKey.c_str() ) == 0;
        }
    } else {
        if( rhs._sortKey.empty() ) {
            return wstricmp( _sortKey.c_str(), rhs._text.c_str() ) == 0;
        } else {
            return wstricmp( _sortKey.c_str(), rhs._sortKey.c_str() ) == 0;
        }
    }
}
/***************************************************************************/
bool IndexItem::operator==( const std::wstring& rhs ) const
{
    if( _sortKey.empty() ) {
        return wstricmp( _text.c_str(), rhs.c_str() ) == 0;
    } else {
        return wstricmp( _sortKey.c_str(), rhs.c_str() ) == 0;
    }
}
/***************************************************************************/
bool IndexItem::operator<( const IndexItem& rhs ) const
{
    if( _sortKey.empty() ) {
        if( rhs._sortKey.empty() ) {
            return wstricmp( _text.c_str(), rhs._text.c_str() ) < 0;
        } else {
            return wstricmp( _text.c_str(), rhs._sortKey.c_str() ) < 0;
        }
    } else {
        if( rhs._sortKey.empty() ) {
            return wstricmp( _sortKey.c_str(), rhs._text.c_str() ) < 0;
        } else {
            return wstricmp( _sortKey.c_str(), rhs._sortKey.c_str() ) < 0;
        }
    }
}
/***************************************************************************/
//return <0 if s < t, 0 if s == t, >0 if s > t
int IndexItem::wstricmp( const wchar_t *s, const wchar_t *t ) const
{
    wchar_t c1( std::towupper( *s ) );
    wchar_t c2( std::towupper( *t ) );
    while( c1 == c2 && c1 != L'\0' ) {
        ++s;
        ++t;
        c1 = std::towupper( *s );
        c2 = std::towupper( *t );
    }
    return( c1 - c2 );
}/***************************************************************************/
//calculate size of text
//write header
//variable length data follows:
//if sortKey bit set
//  char size2                              //size of sortKey text
//  char sortText[size2]                    //sort key text
//char indexText[size or size-(size2+1)];   //index word [not zero-terminated]
//unsigned long synonyms[synonymCount];     //32 bit file offsets to synonyms referencing this word
//
// NOTE: IBM IPFC has some strange limit on index text, it looks like limit is 128 bytes
//          we use limit 255 bytes for index item
//
IndexItem::dword IndexItem::write( OutFile* out )
{
    std::string buffer1;
    std::size_t length1( 0 );
    std::string buffer2( out->wtomb_string( _text ) );
    std::size_t length2 = buffer2.size();
    if( _hdr.sortKey ) {
        buffer1 = out->wtomb_string( _sortKey );
        length1 = buffer1.size() + 1;   // add len byte
        if( length1 > 255 ) {
            length1 = 255;
        }
    }
    if( length1 + length2 > 255 )
        length2 = 255 - length1;
    _hdr.size = static_cast< byte >( length1 + length2 );
    _hdr.synonymCount = static_cast< byte >( _synonyms.size() );
    if( out->write( &_hdr, sizeof( IndexHeader ), 1 ) )
        throw FatalError( ERR_WRITE );
    if( _hdr.sortKey ) {
        length1--;
        if( out->put( static_cast< byte >( length1 ) ) )
            throw FatalError( ERR_WRITE );
        if( length1 > 0 ) {
            if( out->write( buffer1.data(), sizeof( char ), length1 ) ) {
                throw FatalError( ERR_WRITE );
            }
        }
    }
    if( out->write( buffer2.data(), sizeof( char ), length2 ) )
        throw FatalError( ERR_WRITE );
    if( !_synonyms.empty() ) {
        if( out->write( _synonyms.data(), sizeof( dword ), _synonyms.size() ) ) {
            throw FatalError( ERR_WRITE );
        }
    }
    return( static_cast< dword >( sizeof( IndexHeader ) + _hdr.size + _synonyms.size() * sizeof( dword ) ) );
}
