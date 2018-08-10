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
* Description:  A page or panel data
*
****************************************************************************/


#include "wipfc.hpp"
#include "page.hpp"
#include "cell.hpp"
#include "document.hpp"
#include "errors.hpp"
#include "hn.hpp"
#include "outfile.hpp"


void Page::buildTOC()
{
    ( *( _elements.begin() ))->buildTOC( this );
}
/***************************************************************************/
void Page::buildLocalDictionary( OutFile *out )
{
    _out = out;
    bool autoSpacing( true );
    _currentCell = new Cell( _document->maxLocalDictionarySize() );
    _document->addCell( _currentCell );
    _cells.push_back( static_cast< word >( _currentCell->index() ) );
    ++_toc.cellCount;
    for( ElementIter itr = _elements.begin(); itr != _elements.end(); ++itr ) {
        std::pair< bool, bool > flags( ( *itr )->buildLocalDict( this ) );
        if( flags.first ) {
            _currentCell = new Cell( _document->maxLocalDictionarySize() );
            _document->addCell( _currentCell );
            _cells.push_back( static_cast< word >( _currentCell->index() ) );
            ++_toc.cellCount;
            if( !autoSpacing )          //autoSpacing can't cross a cell boundry
                _currentCell->addByte( 0xFC );   //so turn it off so we can turn
            flags = ( *itr )->buildLocalDict( this ); //it back on later
        }
        if( flags.second )
            autoSpacing = !autoSpacing;
        _currentCell->addElement( *itr );
    }
}
/***************************************************************************/
bool Page::addWord( GlobalDictionaryWord* wordent )
{
    if( wordent ) {    //can be 0 for unrecognized entity references
        if( _currentCell->dictFull() )
            return( true );
        _currentCell->addWord( wordent->index() );
        wordent->onPage( _idx );
    }
    return( false );
}
/***************************************************************************/
//Write a TOC entry
Page::dword Page::write( OutFile *out )
{
    std::size_t tocsize( sizeof( TocEntry ) + _toc.cellCount * sizeof( word ) );
    if( _toc.extended ) {
        tocsize += sizeof( ExtTocEntry );
        if( _etoc.setPos )
            tocsize += sizeof( PageOrigin );
        if( _etoc.setSize )
            tocsize += sizeof( PageSize );
        if( _etoc.setStyle )
            tocsize += sizeof( PageStyle );
        if( _etoc.setGroup )
            tocsize += sizeof ( PageGroup );
        if( _etoc.setCtrl ) {
            tocsize += sizeof( PageControls );
        }
    }
    // convert title to mb
    std::string title;
    out->wtomb_string( _title, title );
    if( tocsize + title.size() > 255 ) {
        Hn* hn( static_cast< Hn* >( *( _elements.begin() ) ) );
        hn->printError( ERR2_TEXTTOOLONG );
        title.erase( 255 - tocsize );  //write only part of title
    }
    tocsize += title.size();
    _toc.size = static_cast< byte >( tocsize );
    dword pos = _toc.write( out );
    if( _toc.extended ) {
        _etoc.write( out );
        if( _etoc.setPos )
            _origin.write( out );
        if( _etoc.setSize )
            _size.write( out );
        if( _etoc.setStyle )
            _style.write( out );
        if( _etoc.setGroup )
            _group.write( out );
        if( _etoc.setCtrl ) {
            _controls.write( out );
        }
    }
    if( out->write( &_cells[0], sizeof( word ), _cells.size() ) )
        throw FatalError( ERR_WRITE );
    if( !_title.empty() ) {
        if( out->write( _title.c_str(), sizeof( byte ), _title.size() ) ) {
            throw FatalError( ERR_WRITE );
        }
    }
    return( pos );
}
/***************************************************************************/
// byte size
// word parent_toc_index
// word child_toc_index
Page::dword Page::writeChildren( OutFile *out ) const
{
    byte bytes = 0;
    if( !_children.empty() ) {
        bytes = static_cast< byte >( sizeof( byte ) + sizeof( _idx ) + _children.size() * sizeof( std::vector< word >::value_type ) );
        if( out->put( bytes ) )
            throw FatalError( ERR_WRITE );
        if( out->put( _idx ) )
            throw FatalError( ERR_WRITE );
        if( out->write( &_children[0], sizeof( std::vector< word >::value_type ), _children.size() ) ) {
            throw FatalError( ERR_WRITE );
        }
    }
    return( bytes );
}
