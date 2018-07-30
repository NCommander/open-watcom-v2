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
* Description:  Dictionary (vocabulary) data
*
****************************************************************************/


#include "wipfc.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include "gdict.hpp"
#include "errors.hpp"

GlobalDictionary::~GlobalDictionary()
{
    for( WordIter iter = words.begin(); iter != words.end(); ++iter ) {
        delete *iter;
    }
}
/***************************************************************************/
GlobalDictionaryWord* GlobalDictionary::insert( GlobalDictionaryWord *wordent )
{
    std::pair< WordIter, bool > status( words.insert( wordent ) );
    if( !status.second )
        delete wordent;
    return( *status.first );
}
/***************************************************************************/
GlobalDictionaryWord* GlobalDictionary::insert( const std::wstring& wordtxt )
{
    return( insert( new GlobalDictionaryWord( wordtxt ) ) );
}
/***************************************************************************/
// Call after parsing, but before local dictionary construction
void GlobalDictionary::convert( std::size_t count )
{
    word index = 0;
    for( WordIter iter = words.begin(); iter != words.end(); ++iter, ++index ) {
        (*iter)->setIndex( index );
        (*iter)->setPages( count );
    }
}
/***************************************************************************/
STD1::uint16_t GlobalDictionary::findIndex( const std::wstring& wordtxt )
{
    GlobalDictionaryWord wordent( wordtxt );
    return( findIndex( &wordent ) );
}
/***************************************************************************/
GlobalDictionaryWord* GlobalDictionary::findWord( const std::wstring& wordtxt )
{
    GlobalDictionaryWord wordent( wordtxt );
    return( findWord( &wordent ) );
}
/***************************************************************************/
STD1::uint32_t GlobalDictionary::write( std::FILE *out )
{
    dword start = std::ftell( out );
    for( ConstWordIter itr = words.begin(); itr != words.end(); ++itr )
        bytes += (*itr)->writeWord( out );
    return( start );
}
/***************************************************************************/
bool GlobalDictionary::buildFTS()
{
    bool big( false );
    for( ConstWordIter itr = words.begin(); itr != words.end(); ++itr ) {
        (*itr)->buildFTS();
        if( (*itr)->isBigFTS() ) {
            big = true;
        }
    }
    return( big );
}
/***************************************************************************/
STD1::uint32_t GlobalDictionary::writeFTS( std::FILE *out, bool big )
{
    dword start = std::ftell( out );
    for( ConstWordIter itr = words.begin(); itr != words.end(); ++itr )
        ftsBytes += (*itr)->writeFTS( out, big );
    return( start );
}
