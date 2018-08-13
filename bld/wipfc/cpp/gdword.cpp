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
* Description:  A single word in the Dictionary (vocabulary) data
*
* Each entry is formatted as
* struct {
*     unsigned char   size;
*     char            text[size - 1];
*     } GlobalDictinoaryWord;
*
****************************************************************************/


#include "wipfc.hpp"
#include <cwctype>
#include <cstdlib>
#include "gdword.hpp"
#include "errors.hpp"
#include "util.hpp"
#include "outfile.hpp"


void GlobalDictionaryWord::toUpper()
{
    wchar_t ch;
    for( std::size_t count = 0; count < _text.size(); ++count ) {
        ch = std::towupper( _text[ count ] );
        _text[ count ] = ch;
    }
}
/***************************************************************************/
GlobalDictionaryWord::dword GlobalDictionaryWord::writeWord( OutFile* out ) const
{
    std::string buffer;
    out->wtomb_string( _text, buffer );
    if( buffer.size() > ( 255 - 1 ) )
        buffer.erase( 255 - 1 );
    std::size_t length = buffer.size() + 1;
    if( out->put( static_cast< byte >( length ) ) )
        throw FatalError( ERR_WRITE );
    if( out->write( buffer.data(), sizeof( char ), length - 1 ) )
        throw FatalError( ERR_WRITE );
    return( static_cast< dword >( length ) );
}
/***************************************************************************/
bool GlobalDictionaryWord::operator<( const GlobalDictionaryWord& rhs ) const
{
    int value( wstricmp( _text.c_str(), rhs._text.c_str() ) );
    if( value == 0 )
        value = std::wcscmp( _text.c_str(), rhs._text.c_str() );
    return value < 0;
}
/***************************************************************************/
//return <0 if s < t, 0 if s == t, >0 if s > t
int GlobalDictionaryWord::wstricmp( const wchar_t *s, const wchar_t *t ) const
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
}
