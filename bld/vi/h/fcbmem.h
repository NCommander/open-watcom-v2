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
* Description:  Internal FCB memory interface.
*
****************************************************************************/


/* fcbblock.c */
void    GiveBackBlock( long, unsigned char * );
bool    GetNewBlock( long *, unsigned char *, int );
size_t  MakeWriteBlock( fcb * );

/* fcbdisk.c */
vi_rc   SwapToMemoryFromDisk( fcb * );
vi_rc   SwapToDisk( fcb * );
void    GiveBackSwapBlock( long );
void    SwapFileClose( void );
void    SwapBlockInit( int );

/* fcbems.c */
vi_rc   EMSBlockTest( unsigned short );
void    EMSBlockRead( long, void *, size_t );
void    EMSBlockWrite( long , void *, size_t );
vi_rc   EMSGetBlock( long * );
vi_rc   SwapToEMSMemory( fcb * );
vi_rc   SwapToMemoryFromEMSMemory( fcb * );
void    EMSInit( void );
void    EMSFini( void );
void    GiveBackEMSBlock( long );
void    EMSBlockInit( int );

/* fcbswap.c */
void    SwapFcb( fcb * );
vi_rc   RestoreToNormalMemory( fcb *, size_t );

/* fcbxmem.c */
vi_rc   SwapToExtendedMemory( fcb * );
vi_rc   SwapToMemoryFromExtendedMemory( fcb * );
void    XMemInit( void );
void    XMemFini( void );
void    GiveBackXMemBlock( long );

/* fcbxms.c */
vi_rc   XMSBlockTest( unsigned short );
void    XMSBlockRead( long, void *, size_t );
void    XMSBlockWrite( long , void *, size_t );
vi_rc   XMSGetBlock( long * );
vi_rc   SwapToXMSMemory( fcb * );
vi_rc   SwapToMemoryFromXMSMemory( fcb * );
void    XMSInit( void );
void    XMSFini( void );
void    GiveBackXMSBlock( long );
void    XMSBlockInit( int );
