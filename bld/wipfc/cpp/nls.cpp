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
* Description:  NLS data
*
****************************************************************************/


#include "wipfc.hpp"
#include <cstdlib>
#include <cstring>
#include <climits>
#if defined( __UNIX__ ) || defined( __APPLE__ )
    #include <clocale>
#else
    #include <mbctype.h>
#endif
#include "errors.hpp"
#include "env.hpp"
#include "nls.hpp"
#include "util.hpp"
#include "ipffile.hpp"


Nls::Nls( const char *loc ) : _bytes( 0 )
/***************************************/
{
    _sbcsG._type = WIPFC::GRAPHIC;
    _dbcsG._type = WIPFC::GRAPHIC;
    _sbcsT.setDefaultBits( WIPFC::TEXT );
    _sbcsG.setDefaultBits( WIPFC::GRAPHIC );
    setLocalization( loc );
}

void Nls::readEntityFile( const std::string& sfname )
/***************************************************/
{
    std::wstring    wfname;
    std::wstring    wbuffer;

    def_mbtow_string( sfname, wfname );
    IpfFile *ipff = new IpfFile( sfname, &wfname );
    if( ipff == NULL )
        throw FatalError( ERR_COUNTRY );
    while( ipff->gets( wbuffer ) ) {
        if( wbuffer[0] == L'\0' )
            continue;               //skip blank lines
        wchar_t c = wbuffer[0];
        wbuffer.erase( 0, 1 );
        _entityMap.insert( std::map< std::wstring, wchar_t >::value_type( wbuffer, c ) );
    }
    delete ipff;
}

void Nls::readNLSFile( const std::string& sfname )
/************************************************/
{
    bool                    doGrammar( false );
    std::wstring            wfname;
    std::wstring            wbuffer;
    std::string::size_type  pos;

    def_mbtow_string( sfname, wfname );
    IpfFile *ipff = new IpfFile( sfname, &wfname );
    while( ipff->gets( wbuffer ) ) {
        if( wbuffer[0] == L'\0' )
            continue;               //skip blank lines
        if( wbuffer[0] == L'#' )
            continue;               //skip comments
        if( (pos = wbuffer.find( L'=' )) != std::wstring::npos ) {
            std::wstring keyword = wbuffer.substr( 0, pos );
            std::wstring value = wbuffer.substr( pos + 1 );
            if( doGrammar ) {
                if( keyword == L"Words" ) {
                    processGrammar( value );
                } else if ( keyword == L"RemoveNL" ) {
                    //FIXME: exclude these values from sbcs/dbcs table?
                }
            } else if( keyword == L"Note" ) {
                killQuotes( value );
                _noteText = value;
            } else if( keyword == L"Caution" ) {
                killQuotes( value );
                _cautionText = value;
            } else if( keyword == L"Warning" ) {
                killQuotes( value );
                _warningText = value;
            } else if( keyword == L"Reference" ) {
                killQuotes( value );
                _referenceText = value;
            } else if( keyword == L"olChars" ) {
                _olCh = value;
            } else if( keyword == L"olClose1" ) {
                _olClosers[0] = value;
            } else if( keyword == L"olClose2" ) {
                _olClosers[1] = value;
            } else if( keyword == L"ulItemId1" ) {
                _ulBul[0] = value;
            } else if( keyword == L"ulItemId2" ) {
                _ulBul[1] = value;
            } else if( keyword == L"ulItemId3" ) {
                _ulBul[2] = value;
            } else if( keyword == L"cgraphicFontFaceName" ) {
                killQuotes( value );
                _cgraphicFont.setFaceName( value );
            } else if( keyword == L"cgraphicFontWidth" ) {
                _cgraphicFont.setWidth( static_cast< word >( std::wcstol( value.c_str(), 0, 10 ) ) );
            } else if( keyword == L"cgraphicFontHeight" ) {
                _cgraphicFont.setHeight( static_cast< word >( std::wcstol( value.c_str(), 0, 10 ) ) );
            } else {
                // error: unknown keyword
            }
        } else if( wbuffer == L"Grammar" ) {
            doGrammar = true;
        } else if( wbuffer == L"eGrammar" ) {
            doGrammar = false;
        } else {
            // error: unknown keyword
        }
    }
    delete ipff;
}

void Nls::setLocalization( const char *loc )
/******************************************/
{
    set_document_data_codepage( loc );
    readNLSFile( _country.nlsFileName() );
    readEntityFile( _country.entityFileName() );
}

void Nls::addGrammarItem( wchar_t chr1, wchar_t chr2 )
/****************************************************/
{
    for( wchar_t c = chr1; c <= chr2; ++c )
        _grammarChars += c;
    _dbcsT._ranges.push_back( static_cast< word >( chr1 ) );
    _dbcsT._ranges.push_back( static_cast< word >( chr2 ) );
}

void Nls::processGrammar( const std::wstring& wbuffer )
/*****************************************************/
{
    if( _grammarChars.empty() ) {
        _grammarChars.reserve( 26 + 26 + 10 );
    }
    std::wstring::size_type pos;
    std::wstring::size_type start = 0;
    while( (pos = wbuffer.find( L'+', start )) != std::wstring::npos ) {
        if( pos - start > 1 ) {
            // characters range "chr1-chr2"
            // change this loop if we use RegExp
            addGrammarItem( wbuffer[start + 0], wbuffer[start + 2] );
        } else {
            // single character "chr"
            addGrammarItem( wbuffer[start + 0], wbuffer[start + 0] );
        }
        start = pos + 1;
    }
    pos = wbuffer.length();
    if( pos > start ) {
        if( pos - start > 1 ) {
            // characters range "chr1-chr2"
            // change this loop if we use RegExp
            addGrammarItem( wbuffer[start + 0], wbuffer[start + 2] );
        } else {
            // single character "chr"
            addGrammarItem( wbuffer[start + 0], wbuffer[start + 0] );
        }
    }
}

wchar_t Nls::entityChar( const std::wstring& key )
/************************************************/
{
    EntityIter pos( _entityMap.find( key ) );
    if( pos == _entityMap.end() )
        throw Class2Error( ERR2_SYMBOL );
    return pos->second;
}

STD1::uint32_t Nls::write( std::FILE *out )
/*****************************************/
{
    _bytes = _country.size();
    dword start = _country.write( out );
    if( _country.useDBCS() ) {
        _dbcsT.write( out );
        _bytes += _dbcsT._size;
        _dbcsG.write( out );
        _bytes += _dbcsG._size;
    } else {
        _sbcsT.write( out );
        _bytes += _sbcsT._size;
        _sbcsG.write( out );
        _bytes += _sbcsG._size;
    }
    return( start );
}

void Nls::SbcsGrammarDef::setDefaultBits( WIPFC::NLSRecType rectype )
/*******************************************************************/
{
    static const unsigned char defbits[2][32] = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc0,
          0x7f, 0xff, 0xff, 0xe0, 0x7f, 0xff, 0xff, 0xe0,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x7f, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
    };
    std::memcpy( this->_bits, &defbits[rectype - WIPFC::TEXT][0], 32 );
}

STD1::uint32_t Nls::SbcsGrammarDef::write( std::FILE *out ) const
/***************************************************************/
{
    dword start = std::ftell( out );
    if( std::fwrite( &_size, sizeof( _size ), 1, out ) != 1 )
        throw FatalError( ERR_WRITE );
    byte type = static_cast< byte >( _type );
    if( std::fwrite( &type, sizeof( type ), 1, out ) != 1 )
        throw FatalError( ERR_WRITE );
    if( std::fwrite( &_format, sizeof( _format ), 1, out ) != 1 )
        throw FatalError( ERR_WRITE );
    if( std::fwrite( _bits, sizeof( _bits[0] ), sizeof( _bits ) / sizeof( _bits[0] ), out ) != sizeof( _bits ) / sizeof( _bits[0] ) )
        throw FatalError( ERR_WRITE );
    return( start );
}

STD1::uint32_t Nls::DbcsGrammarDef::write( std::FILE *out )
/*********************************************************/
{
    dword start = std::ftell( out );
    _size = static_cast< word >( sizeof( word ) + 2 * sizeof( byte ) + _ranges.size() * sizeof( word ) );
    if( std::fwrite( &_size, sizeof( _size ), 1, out ) != 1 )
        throw FatalError( ERR_WRITE );
    byte type = static_cast< byte >( _type );
    if( std::fwrite( &type, sizeof( type ), 1, out ) != 1 )
        throw FatalError( ERR_WRITE );
    if( std::fwrite( &_format, sizeof( _format ), 1, out ) != 1 )
        throw FatalError( ERR_WRITE );
    for( std::vector< word >::const_iterator itr = _ranges.begin(); itr != _ranges.end(); ++itr ) {
        if( std::fwrite( &(*itr), sizeof( word ), 1, out ) != 1 ) {
            throw FatalError( ERR_WRITE );
        }
    }
    return( start );
}

//
// Following code is responsible for document data conversion UNICODE<->MBCS
//  in dependency on required code page (default is cp=850)
//
// TODO! MBCS<->UNICODE conversion for mbtow_char and wtomb_char must be setup
// instead of existing code which rely on host OS locale support.
// By example proper characters encoding for US INF Documentation files is
// DOS codepage 850, but on Linux it is handled as ISO-8859-1 or UTF-8 in
// dependency how host locale are configured. It is wrong!
// Correct solution is to use DOS codepage 850 for US on any host OS.
// It requires to define appropriate MBCS<->UNICODE conversion tables as part of WIPFC.

void Nls::set_document_data_codepage( const char *loc )
/*****************************************************/
{
    _country.nlsConfig( loc );
    // TODO! It is wrong code, but we doesn't have any better for now
#if defined( __UNIX__ ) || defined( __APPLE__ )
    std::setlocale( LC_ALL, loc );
#else
    _setmbcp( _country.codePage() );
#endif
}

static int wtomb_char( char *mbc, wchar_t wc )
/********************************************/
{
    // TODO! must be converted by selected UNICODE->MBCS conversion table
    // which is independent from the host user locale
    return( std::wctomb( mbc, wc ) );
}

std::size_t Nls::wtomb_cstring( char *dst_mbc, const wchar_t *src_wc, std::size_t len )
/*************************************************************************************/
{
    std::size_t dst_len = 0;
    char        mbc[MB_LEN_MAX + 1];
    int         bytes;

    while( len > 0 && *src_wc != L'\0' ) {
        bytes = wtomb_char( mbc, *src_wc );
        if( bytes == -1 || (unsigned)bytes > len )
            return( ERROR_CNV );
        std::memcpy( dst_mbc, mbc, bytes );
        dst_mbc += bytes;
        dst_len += bytes;
        len -= bytes;
        src_wc++;
    }
    *dst_mbc = '\0';
    return( dst_len );
}

void Nls::wtomb_string( const std::wstring& input, std::string& output )
/**********************************************************************/
{
    for( std::size_t index = 0; index < input.size(); ++index ) {
        char ch[ MB_LEN_MAX + 1 ];
        int  bytes( wtomb_char( &ch[ 0 ], input[ index ] ) );
        if( bytes == -1 )
            throw FatalError( ERR_T_CONV );
        ch[ bytes ] = '\0';
        output += ch;
    }
}
