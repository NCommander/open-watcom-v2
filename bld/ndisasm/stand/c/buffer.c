/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "dis.h"
#include "global.h"
#include "buffer.h"
#include "print.h"
#include "formasm.h"
#include "labproc.h"

extern dis_format_flags         DFormat;

static unsigned                 OutputPos = 0;
static char                     Buffer[BUFFER_LEN] = {0};
static char                     IntermedBuffer[BUFFER_LEN] = {0};

void FmtHexNum( char *buff, unsigned prec, dis_value value, bool no_prefix )
{
    char        *src;
    char        *dst;
    const char  *fmt;
    int         len;

    if( (DFormat & DFF_ASM) && IsMasmOutput() ) {
        if( ( value.u._32[I64LO32] == 0 ) && ( value.u._32[I64HI32] == 0 ) && ( prec == 0 ) ) {
            strcpy( buff, "0" );
        } else {
            fmt = ( no_prefix ) ? "%*.*lxH" : "0%*.*lxH";
            if( value.u._32[I64HI32] == 0 ) {
                len = 0;
            } else {
                if( prec > 8 ) {
                    len = sprintf( buff, fmt, prec - 8, prec - 8, value.u._32[I64HI32] );
                    prec = 8;
                } else {
                    len = sprintf( buff, fmt, 0, 0, value.u._32[I64HI32] );
                }
                fmt = "%*.*lxH";
            }
            sprintf( buff + len, fmt, prec, prec, value.u._32[I64LO32] );
        }
        /* don't need the extra leading zero, squeeze it out */
        for ( src = dst = buff; *src != '\0'; src++ ) {
            if ( dst != buff || src[0] != '0' || !isdigit( src[1] ) ) {
                *dst = *src;
                dst++;
            }
        }
        *dst = '\0';
        if ( buff[1] == 'H' ) {
            buff[1] = '\0';
        }
    } else {
        if( ( value.u._32[I64LO32] == 0 ) && ( value.u._32[I64HI32] == 0 ) && ( prec == 0 ) ) {
            strcpy( buff, "0x0" );
        } else {
            fmt = ( no_prefix ) ? "%*.*lx" : "0x%*.*lx";
            if( value.u._32[I64HI32] == 0 ) {
                len = 0;
            } else {
                if( prec > 8 ) {
                    len = sprintf( buff, fmt, prec - 8, prec - 8, value.u._32[I64HI32] );
                    prec = 8;
                } else {
                    len = sprintf( buff, fmt, 0, 0, value.u._32[I64HI32] );
                }
                fmt = "%*.*lx";
            }
            sprintf( buff + len, fmt, prec, prec, value.u._32[I64LO32] );
        }
    }
}

void BufferAlignToTab( unsigned pos )
// align the buffer to a particular tab position
{
    unsigned            num_tabs;
    unsigned            loop;

    if( pos * TAB_WIDTH < OutputPos ) {
        BufferConcatNL();
        num_tabs = pos;
    } else {
        num_tabs = (pos * TAB_WIDTH - OutputPos + TAB_WIDTH - 1 ) / TAB_WIDTH;
    }
    for( loop = 0; loop < num_tabs; loop++ ) {
        IntermedBuffer[loop] = '\t';
    }
    IntermedBuffer[loop] = '\0';
    strcat( Buffer, IntermedBuffer );
    OutputPos = pos * TAB_WIDTH;
}

static void updateOutputPos( const char *string )
// update the position of the last character as it will be seen in output
{
    if( string == NULL ) return;
    while( *string ) {
        if( *string == '\n' ) {
            OutputPos = 0;
        } else if( *string == '\t' ) {
            OutputPos = ((OutputPos / 8) + 1) * 8;
        } else {
            OutputPos++;
        }
        string++;
    }
}

void BufferConcat( const char *string )
// concatenate a string on the end of the buffer
{
    strcat( Buffer, string );
    updateOutputPos( string );
}

void BufferConcatNL( void )
// concatentate a \n on the end of the buffer
{
    strcat( Buffer, "\n" );
    OutputPos = 0;
}

size_t BufferStore( const char *format, ... )
// do the equivalent of a printf to the end of the buffer
{
    va_list     arg_list;
    size_t      len;

    va_start( arg_list, format );
    len = vsprintf( IntermedBuffer, format, arg_list );
    va_end( arg_list );
    strcat( Buffer, IntermedBuffer );
    updateOutputPos( IntermedBuffer );
    return( len );
}

size_t BufferMsg( int resourceid )
// concatenate a message from the resource file
{
    char        buff[MAX_RESOURCE_SIZE];

    if( MsgGet( resourceid, buff ) ) {
        BufferConcat( buff );
        return( strlen( buff ) );
    }
    return( 0 );
}

void BufferPrint( void )
// print the buffer to current output destination
{
    Print( Buffer );
    *Buffer = 0;
}

void BufferHex( unsigned prec, dis_value value )
{
    FmtHexNum( IntermedBuffer, prec, value, false );
    BufferConcat( IntermedBuffer );
}

void BufferQuoteName( const char *name )
{
    if( NeedsQuoting( name ) ) {
        BufferConcat( "`" );
        BufferConcat( name );
        BufferConcat( "`" );
    } else {
        BufferConcat( name );
    }
}
