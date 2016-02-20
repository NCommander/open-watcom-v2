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
* Description:  Remote file access.
*
****************************************************************************/


//#define LOGGING 1

#ifdef LOGGING
#include <stdio.h>
#endif
#ifdef __NT__
#include <windows.h>
#endif
#include "wio.h"
#include "dbgdefn.h"
#if !defined( BUILD_RFX )
#include "dbgdata.h"
#else
#include "rfxdata.h"
#endif
#include "dbgio.h"
#include "trpfile.h"
#include "filermt.h"
#include "filelcl.h"
#include "trapglbl.h"
#include "remfile.h"
#include "trpld.h"


extern trap_elen        MaxPacketLen;

static trap_shandle     SuppFileId = 0;

file_components         RemFile;

#ifdef LOGGING
static FILE    *logf;
#endif

/* Remote file "cache" - correlates remote and local file handles */
typedef struct _fcache_t {
    int         locfile;
    sys_handle  remhandle;
} fcache_t;

#define CACHED_HANDLES    16
static fcache_t    fcache[CACHED_HANDLES];

#define SUPP_FILE_SERVICE( in, request )    \
    in.supp.core_req    = REQ_PERFORM_SUPPLEMENTARY_SERVICE; \
    in.supp.id          = SuppFileId;       \
    in.req              = request;

/* Return local handle of remote file equivalent */
int GetCachedHandle(sys_handle remote)
{
    int i;

    for( i = 0; i < CACHED_HANDLES; i++ ) {
        if( fcache[i].remhandle == remote ) {
            return( fcache[i].locfile );
        }
    }
    return( -1 );
}

/* Initialize local/remote handle cache */
void InitHandleCache( void )
{
    int     i;

#ifdef LOGGING
    logf = fopen("wdrem.log", "wt");
    fprintf(logf, "InitHandleCache called\n");
#endif

    for( i = 0; i < CACHED_HANDLES; i++ ) {
        fcache[i].remhandle = -1;
        fcache[i].locfile   = -1;
    }
}

/* Add entry for local/remote "cached" file */
int AddCachedHandle( int local, sys_handle remote )
{
    int     i = 0;

    while( i < CACHED_HANDLES ) {
        if( fcache[i].locfile == -1 ) {
            fcache[i].remhandle = remote;
            fcache[i].locfile   = local;
            return( 0 );
        }
        i++;
    }
    return( -1 );
}

/* Remove cached file entry from the list */
int DelCachedHandle( int local )
{
    int     i = 0;

    while( i < CACHED_HANDLES ) {
        if( fcache[i].locfile == local ) {
            fcache[i].remhandle = -1;
            fcache[i].locfile   = -1;
            return( 0 );
        }
        i++;
    }
    return( -1 );
}

bool InitFileSupp( void )
{
    file_get_config_req acc;

    InitHandleCache();

    SuppFileId = GetSuppId( FILE_SUPP_NAME );
    if( SuppFileId == 0 )
        return( false );
    SUPP_FILE_SERVICE( acc, REQ_FILE_GET_CONFIG );
    TrapSimpAccess( sizeof( acc ), &acc, sizeof( RemFile ), &RemFile );
    return( true );
}

bool HaveRemoteFiles( void )
{
    return( SuppFileId != 0 );
}

//NYI: The 'bool executable' should be changed to allow different file types
size_t RemoteStringToFullName( bool executable, const char *name, char *res,
                                 trap_elen res_len )
{
    in_mx_entry         in[2];
    mx_entry            out[2];
    file_string_to_fullpath_req acc;
    file_string_to_fullpath_ret ret;
    file_handle         fh;
#ifdef __NT__
    char short_filename[MAX_PATH + 1] = "";
#endif

    if( SuppFileId == 0 ) {
        fh = LclStringToFullName( name, strlen( name ), res );
        if( fh == NIL_HANDLE )
            return( 0 );
        FileClose( fh );
        return( strlen( res ) );
    }
    SUPP_FILE_SERVICE( acc, REQ_FILE_STRING_TO_FULLPATH );
    acc.file_type = ( executable ? TF_TYPE_EXE : TF_TYPE_PRS );
    in[0].ptr = &acc;
    in[0].len = sizeof( acc );
    in[1].ptr = (void *)name;
#ifdef __NT__
    // check whether short filename is necessary
    switch( SysConfig.os ) {
    case MAD_OS_AUTOCAD:
    case MAD_OS_DOS:
    case MAD_OS_RATIONAL:
    case MAD_OS_PHARLAP:
    case MAD_OS_WINDOWS:
        // convert long file name to short "DOS" compatible form
        {
            GetShortPathNameA( name, short_filename, MAX_PATH );
            if( *short_filename != '\0' ) {
                in[1].ptr = short_filename;
            }
        }
        break;
    }
#endif
    in[1].len = (trap_elen)( strlen( in[1].ptr ) + 1 );
    out[0].ptr = &ret;
    out[0].len = sizeof( ret );
    out[1].ptr = res;
    out[1].len = res_len;
    TrapAccess( 2, in, 2, out );
    CONV_LE_32( ret.err );
    if( ret.err != 0 ) {
        *res = NULLCHAR;
        return( 0 );
    } else {
        return( strlen( res ) );
    }
}

sys_handle RemoteOpen( const char *name, open_access mode )
{
    in_mx_entry         in[2];
    mx_entry            out[1];
    file_open_req       acc;
    file_open_ret       ret;
    int                 locfile;

    if( SuppFileId == 0 )
        return( NIL_SYS_HANDLE );

    SUPP_FILE_SERVICE( acc, REQ_FILE_OPEN );
    acc.mode = 0;
    if( mode & OP_READ )
        acc.mode |= TF_READ;
    if( mode & OP_WRITE )
        acc.mode |= TF_WRITE;
    if( mode & OP_CREATE ) {
        acc.mode |= TF_CREATE;
        if( mode & OP_EXEC ) {
            acc.mode |= TF_EXEC;
        }
    }
    in[0].ptr = &acc;
    in[0].len = sizeof( acc );
    in[1].ptr = name;
    in[1].len = (trap_elen)( strlen( name ) + 1 );
    out[0].ptr = &ret;
    out[0].len = sizeof( ret );
    TrapAccess( 2, in, 1, out );
    CONV_LE_32( ret.err );
    CONV_LE_32( ret.handle );
    if( ret.err != 0 ) {
        StashErrCode( ret.err, OP_REMOTE );
        return( NIL_SYS_HANDLE );
    } else {
        /* See if the file is available locally. If so, open it here as
         * well as on the remote machine.
         */
        // TODO: check if remote file is the same!

#ifdef LOGGING
        fprintf( logf, "Trying to open local copy of remote file (remote handle %d)\n", ret.handle );
        fprintf( logf, "%s\n", name );
#endif

        if( (locfile = open(name, O_RDONLY | O_BINARY, 0 )) != -1 ) {
            if(AddCachedHandle( locfile, ret.handle ) != 0 )
                close( locfile );
#ifdef LOGGING
            fprintf(logf, "Success\n", name);
#endif
        }
        return( ret.handle );
    }
}

static size_t doWrite( sys_handle hdl, const void *buff, size_t len )
{
    in_mx_entry             in[2];
    mx_entry                out[1];
    file_write_req          acc;
    file_write_ret          ret;
    size_t                  total;
    trap_elen               piece;

    SUPP_FILE_SERVICE( acc, REQ_FILE_WRITE );
    acc.handle = hdl;
    CONV_LE_32( acc.handle );
    in[0].ptr = &acc;
    in[0].len = sizeof( acc );
    out[0].ptr = &ret;
    out[0].len = sizeof( ret );

    piece = (trap_elen)( MaxPacketLen - sizeof( acc ) );
    total = 0;
    while( len > 0 ) {
        if( piece > len )
            piece = (trap_elen)len;
        in[1].ptr = buff;
        in[1].len = piece;
        TrapAccess( 2, in, 1, out );
        CONV_LE_32( ret.err );
        CONV_LE_16( ret.len );
        if( ret.err != 0 ) {
            StashErrCode( ret.err, OP_REMOTE );
            return( ERR_RETURN );
        }
        total += ret.len;
        if( ret.len != piece )
            break;
        buff = (char *)buff + ret.len;
        len -= ret.len;
    }
    return( total );
}

size_t RemoteWrite( sys_handle hdl, const void *buff, size_t len )
{
    if( SuppFileId == 0 )
        return( 0 );

    return( doWrite( hdl, buff, len ) );
}

static size_t doWriteConsole( const void *buff, size_t len )
{
    in_mx_entry             in[2];
    mx_entry                out[1];
    file_write_console_req  acc;
    file_write_console_ret  ret;
    size_t                  total;
    trap_elen               piece;

    SUPP_FILE_SERVICE( acc, REQ_FILE_WRITE_CONSOLE );
    in[0].ptr = &acc;
    in[0].len = sizeof( acc );
    out[0].ptr = &ret;
    out[0].len = sizeof( ret );

    piece = (trap_elen)( MaxPacketLen - sizeof( acc ) );
    total = 0;
    while( len > 0 ) {
        if( piece > len )
            piece = (trap_elen)len;
        in[1].ptr = buff;
        in[1].len = piece;
        TrapAccess( 2, in, 1, out );
        CONV_LE_32( ret.err );
        CONV_LE_16( ret.len );
        if( ret.err != 0 ) {
            StashErrCode( ret.err, OP_REMOTE );
            return( ERR_RETURN );
        }
        total += ret.len;
        if( ret.len != piece )
            break;
        buff = (char *)buff + ret.len;
        len -= ret.len;
    }
    return( total );
}

size_t RemoteWriteConsole( const void *buff, size_t len )
{
    if( SuppFileId == 0 )
        return( 0 );

    return( doWriteConsole( buff, len ) );
}

size_t RemoteWriteConsoleNL( void )
{
    if( SuppFileId == 0 )
        return( 0 );

    return( doWriteConsole( RemFile.newline, ( RemFile.newline[1] != NULLCHAR ) ? 2 : 1 ) );
}

static size_t doRead( sys_handle hdl, void *buff, size_t len )
{
    in_mx_entry         in[1];
    mx_entry            out[2];
    file_read_req       acc;
    file_read_ret       ret;
    size_t              total;
    trap_elen           piece;
    trap_retval         read_len;

    SUPP_FILE_SERVICE( acc, REQ_FILE_READ );
    acc.handle = hdl;
    CONV_LE_32( acc.handle );
    in[0].ptr = &acc;
    in[0].len = sizeof( acc );
    out[0].ptr = &ret;
    out[0].len = sizeof( ret );

    piece = (trap_elen)( MaxPacketLen - sizeof( file_read_req ) );
    total = 0;
    while( len > 0 ) {
        if( piece > len )
            piece = (trap_elen)len;
        out[1].ptr = buff;
        out[1].len = piece;
        acc.len = piece;
        CONV_LE_16( acc.len );
        read_len = (trap_retval)TrapAccess( 1, in, 2, out ) - sizeof( ret );
        CONV_LE_32( ret.err );
        if( ret.err != 0 ) {
            StashErrCode( ret.err, OP_REMOTE );
            return( ERR_RETURN );
        }
        total += read_len;
        if( read_len != piece )
            break;
        buff = (char *)buff + read_len;
        len -= read_len;
    }
    return( total );
}

size_t RemoteRead( sys_handle hdl, void *buff, size_t len )
{
    int         locfile;

    if( SuppFileId == 0 )
        return( 0 );

    /* Try reading from local copy first */
    locfile = GetCachedHandle( hdl );
    if( locfile != -1 )
        return( LocalRead( locfile, buff, len ) );

    return( doRead( hdl, buff, len ) );
}

unsigned long RemoteSeek( sys_handle hdl, unsigned long pos, seek_method method )
{
    file_seek_req       acc;
    file_seek_ret       ret;
    int                 locfile;

    if( SuppFileId == 0 )
        return( 0 );

    /* Seek on local copy too (if available) */
    locfile = GetCachedHandle( hdl );
    if( locfile != -1 ) {
        lseek( locfile, pos, method );
    }

    SUPP_FILE_SERVICE( acc, REQ_FILE_SEEK );
    acc.handle = hdl;
    /* Magic again! The seek mode mapped exactly to our definition! */
    acc.mode = method;
    acc.pos = pos;
    CONV_LE_32( acc.handle );
    CONV_LE_32( acc.pos );
    TrapSimpAccess( sizeof( acc ), &acc, sizeof( ret ), &ret );
    CONV_LE_32( ret.pos );
    CONV_LE_32( ret.err );
    if( ret.err != 0 ) {
        StashErrCode( ret.err, OP_REMOTE );
        return( ERR_SEEK );
    } else {
        return( ret.pos );
    }
}

error_handle RemoteClose( sys_handle hdl )
{
    file_close_req      acc;
    file_close_ret      ret;
    int                 locfile;

    if( SuppFileId == 0 )
        return( 0 );

    locfile = GetCachedHandle( hdl );
    if( locfile != -1 ) {
        close( locfile );
        DelCachedHandle( locfile );
#ifdef LOGGING
        fprintf( logf, "Closing remote file handle %d\n", hdl );
#endif
    }

    SUPP_FILE_SERVICE( acc, REQ_FILE_CLOSE );
    acc.handle = hdl;
    CONV_LE_32( acc.handle );
    TrapSimpAccess( sizeof( acc ), &acc, sizeof( ret ), &ret );
    CONV_LE_32( ret.err );
    return( StashErrCode( ret.err, OP_REMOTE ) );
}

error_handle RemoteErase( const char *name )
{
    in_mx_entry         in[2];
    mx_entry            out[1];
    file_erase_req      acc;
    file_erase_ret      ret;

    if( SuppFileId == 0 )
        return( 0 );
    SUPP_FILE_SERVICE( acc, REQ_FILE_ERASE );
    in[0].ptr = &acc;
    in[0].len = sizeof( acc );
    in[1].ptr = name;
    in[1].len = (trap_elen)( strlen( name ) + 1 );
    out[0].ptr = &ret;
    out[0].len = sizeof( ret );
    TrapAccess( 2, in, 1, out );
    CONV_LE_32( ret.err );
    return( StashErrCode( ret.err, OP_REMOTE ) );
}

#if !defined( BUILD_RFX )
error_handle RemoteFork( const char *cmd, trap_elen len )
{
    in_mx_entry         in[2];
    mx_entry            out[1];
    file_run_cmd_req    acc;
    file_run_cmd_ret    ret;

    if( SuppFileId == 0 )
        return( 0 );
    SUPP_FILE_SERVICE( acc, REQ_FILE_RUN_CMD );
    acc.chk_size = CheckSize;
    in[0].ptr = &acc;
    in[0].len = sizeof( acc );
    in[1].ptr = (char *)cmd;
    in[1].len = len;
    out[0].ptr = &ret;
    out[0].len = sizeof( ret );
    TrapAccess( 2, in, 1, out );
    CONV_LE_32( ret.err );
    return( StashErrCode( ret.err, OP_REMOTE ) );
}
#endif
