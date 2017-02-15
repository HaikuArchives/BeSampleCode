/* $Id: //depot/blt/util/dfp.h#2 $
**
** Copyright 1998 Brian J. Swetland
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions, and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions, and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
** Distributed Fish Protocol
** Version 1.0
**
** Special Interest Group for Operating Systems
** Association for Computing Machinery
** University of Illinios at Urbana-Champaign
*/

#ifndef _DFP_H
#define _DFP_H

#ifdef _WIN32
#include <sys/types.h>
#include <winsock.h>
#endif

#ifdef NEED_TYPES
typedef unsigned char  uint8;
typedef unsigned short uint16;
#endif

typedef char sint8;

#define DFP_VERSION	        0x0201

#define DEFAULT_DFP_PORT	5049

#define DFP_MIN_SUBTANK_X	0
#define DFP_MAX_SUBTANK_X	3
#define DFP_MIN_SUBTANK_Y	0
#define DFP_MAX_SUBTANK_Y	3

#define DFP_PKT_PING		0
#define DFP_PKT_PONG		1
#define DFP_PKT_SEND_FISH	2
#define DFP_PKT_ACK_FISH	3
#define	DFP_PKT_NACK_FISH	4
#define DFP_PKT_SYNC_FISH	5

/* pack everything along byte boundaries */	
#pragma pack( 1 )

typedef struct 
{
    uint16  version;
    uint8   src_tank_x;
    uint8   src_tank_y;
    uint8   dst_tank_x;
    uint8   dst_tank_y;
    uint8   type;         /* one of DFP_PKT_* */
    uint8   pad;          /* should be 0 */
    uint16  size;         /* size of entire packet */
} dfp_base;

typedef struct
{
    uint8  creator_tank_x;
    uint8  creator_tank_y;
    uint8  timestamp[4];
} dfp_uid;

typedef struct
{
    uint8  x;
    uint8  y;
    sint8  dx;
    sint8  dy;
    uint16 ttl;
    char  name[16];
} dfp_fish;


#define DFP_SIZE_LOCATE   10
#define DFP_SIZE_CONFIRM  16
#define DFP_SIZE_TRANSFER 38


typedef struct
{
    dfp_base base;
} dfp_pkt_locate;

typedef struct 
{
    dfp_base base;
    dfp_uid  uid;
} dfp_pkt_confirm;

typedef struct 
{
    dfp_base base;
    dfp_uid  uid;
    dfp_fish fish;
} dfp_pkt_transfer;

/* restore default packing */
#pragma pack()

#endif /* __DFP_H */
