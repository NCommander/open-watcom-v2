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
* Description:  Process hpn/ehpn tags
*
*   :hp1-:hp9 / :ehp1-:ehp9
*   Cannot nest
*   Note that the treatment here is an extension to allow nesting
*
****************************************************************************/


#include "wipfc.hpp"
#include "hpn.hpp"
#include "cell.hpp"
#include "document.hpp"
#include "errors.hpp"


std::vector< STD1::uint8_t > Hpn::_levelStack;

Hpn::Hpn( Document* d, Element *p, const std::wstring* f, unsigned int r,
          unsigned int c, unsigned int l ) : Element( d, p, f, r, c ),
          _level( static_cast< STD1::uint8_t >( l ) ), _previousLevel( 0 )
{
    if( !_levelStack.empty() ) {
        _previousLevel = _levelStack[_levelStack.size() - 1];
        d->printError( ERR2_NEST );
    }
    _levelStack.push_back( _level );
}
/***************************************************************************/
Lexer::Token Hpn::parse( Lexer* lexer )
{
    Lexer::Token tok( _document->getNextToken() );
    (void)lexer;
    while( tok != Lexer::TAGEND ) {
        if( tok == Lexer::ATTRIBUTE ) {
            _document->printError( ERR1_ATTRNOTDEF );
        } else if( tok == Lexer::FLAG ) {
            _document->printError( ERR1_ATTRNOTDEF );
        } else if( tok == Lexer::ERROR_TAG ) {
            throw FatalError( ERR_SYNTAX );
        } else if( tok == Lexer::END ) {
            throw FatalError( ERR_EOF );
        } else {
            _document->printError( ERR1_TAGSYNTAX );
        }
        tok = _document->getNextToken();
    }
    return _document->getNextToken();    //consume TAGEND
}
/***************************************************************************/
void Hpn::buildText( Cell* cell )
{
    if( _previousLevel ) {
        //kill the previous level
        cell->addByte( 0xFF );      //esc
        cell->addByte( 0x03 );      //size
        if( _previousLevel != 4 && _previousLevel < 8 ) {
            cell->addByte( 0x04 );  //change style
        } else {
            cell->addByte( 0x0D );  //special text color
        }
        cell->addByte( 0x00 );      //default
    }
    cell->addByte( 0xFF );          //esc
    cell->addByte( 0x03 );          //size
    if( _level != 4 && _level < 8 ) {
        cell->addByte( 0x04 );      //change style
        if( _level < 4 ) {
            cell->addByte( static_cast< STD1::uint8_t >( _level ) );
        } else {
            cell->addByte( static_cast< STD1::uint8_t >( _level - 1) );
        }
    } else {
        cell->addByte( 0x0D );      //special text color
        if( _level == 4 ) {
            cell->addByte( 0x01 );
        } else {
            cell->addByte( static_cast< STD1::uint8_t >( _level - 6) );
        }
    }
    if( cell->textFull() ) {
        printError( ERR1_LARGEPAGE );
    }
}
/***************************************************************************/
EHpn::EHpn( Document* d, Element *p, const std::wstring* f, unsigned int r,
            unsigned int c, unsigned int l ) : Element ( d, p, f, r, c ),
            _level( static_cast< STD1::uint8_t >( l ) ), _previousLevel( 0 )
{
    std::vector< STD1::uint8_t >& _levelStack( Hpn::levels() );
    if( _levelStack[_levelStack.size() - 1] != l )
        d->printError( ERR2_NEST );
    _levelStack.pop_back();
    if( !_levelStack.empty() ) {
        d->printError( ERR1_TAGCONTEXT );
        _previousLevel = _levelStack[_levelStack.size() - 1];
    }
}
/***************************************************************************/
Lexer::Token EHpn::parse( Lexer* lexer )
{
    Lexer::Token tok( _document->getNextToken() );
    (void)lexer;
    while( tok != Lexer::TAGEND ) {
        if( tok == Lexer::ATTRIBUTE ) {
            _document->printError( ERR1_ATTRNOTDEF );
        } else if( tok == Lexer::FLAG ) {
            _document->printError( ERR1_ATTRNOTDEF );
        } else if( tok == Lexer::ERROR_TAG ) {
            throw FatalError( ERR_SYNTAX );
        } else if( tok == Lexer::END ) {
            throw FatalError( ERR_EOF );
        } else {
            _document->printError( ERR1_TAGSYNTAX );
        }
        tok = _document->getNextToken();
    }
    return _document->getNextToken();    //consume TAGEND
}
/***************************************************************************/
void EHpn::buildText( Cell* cell )
{
    cell->addByte( 0xFF );          //esc
    cell->addByte( 0x03 );          //size
    if( _level != 4 && _level < 8 ) {
        cell->addByte( 0x04 );      //change style
    } else {
        cell->addByte( 0x0D );      //special text color
    }
    cell->addByte( 0x00 );          //default
    if( _previousLevel ) {
        cell->addByte( 0xFF );      //esc
        cell->addByte( 0x03 );      //size
        if( _previousLevel != 4 && _previousLevel < 8 ) {
            cell->addByte( 0x04 );  //change style
            if( _previousLevel < 4 ) {
                cell->addByte( static_cast< STD1::uint8_t >( _previousLevel ) );
            } else {
                cell->addByte( static_cast< STD1::uint8_t >( _previousLevel - 1) );
            }
        } else {
            cell->addByte( 0x0D );  //special text color
            if( _previousLevel == 4 ) {
                cell->addByte( 0x01 );
            } else {
                cell->addByte( static_cast< STD1::uint8_t >( _previousLevel - 6) );
            }
        }
    }
    if( cell->textFull() ) {
        printError( ERR1_LARGEPAGE );
    }
}
