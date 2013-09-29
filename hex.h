/*
 * eprom_tools: A collection of utilities for use with EPROM programmers
 *
 * Copyright (C) 1995 Mark J. Blair
 *
 * This file is part of eprom_tools.
 *
 *  eprom_tools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  eprom_tools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with eprom_tools.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifndef __hex_h
#define __hex_h

#include <stdio.h>

/* prototypes for the conversion functions */
typedef int WRHEXFUNC(FILE *, FILE *, ULONG, ULONG);
typedef int RDHEXFUNC(FILE *, FILE *, int, ULONG *, ULONG *);
typedef int SCANHEXFUNC(FILE *, ULONG *, ULONG *, ULONG *, ULONG *);

/* array of structures that point to conversion functions */
typedef struct convstruct {
    char	*name;
    char	*desc;
    ULONG	maxaddr;
    RDHEXFUNC	*rd_hex;
    WRHEXFUNC	*wr_hex;
    SCANHEXFUNC	*scan_hex;
    char	*magic;
    int		magic_len;
    int		magic_offset;
} CONVSTRUCT;
extern CONVSTRUCT converters[];

/* maximum address sizes */
#define MAXADDR_INTEL		0x0000ffff
#define MAXADDR_INTEL86		0x000fffff
#define MAXADDR_INTEL32		0xffffffff

/* error codes */
extern int hex_errno;
extern const int hex_nerr;
extern const char *hex_errlist[];
extern void hex_perror(char *);
#define H_ERR_NONE	0	/* no error */
#define H_ERR_ADDR	1	/* address too large */
#define H_ERR_IO	2	/* I/O error */
#define H_ERR_BADHEX	3	/* invalid hex data */
#define H_ERR_RECTYPE	4	/* unknown record type */
#define H_ERR_BADSUM	5	/* bad checksum */
#define H_ERR_ENTRY	6	/* entry address too large for field */
#define ERR(a) hex_errno=(a); return hex_errno

/* hex conversion macros */
extern char _hex2nybble_[];	/* lookup table used by macros */
extern char _nybble2hex_[];	/* lookup table used by macros */
/* char H2C(UCHAR *c) */
#define H2C(c) ((_hex2nybble_[(c)[0]]<<4)|(_hex2nybble_[(c)[1]]))
/* char C2H_H(char c), C2H_L(char c) */
#define C2H_H(c) (_nybble2hex_[((c)>>4)&0xf])
#define C2H_L(c) (_nybble2hex_[(c)&0xf])

/* misc prototypes */
extern int fcat(FILE *, FILE *);

/* Offsets into converters[] for supported formats */
#define FMT_INTEL	0
#define FMT_INTEL86	1
#define FMT_INTEL32	2
#define FMT_UNDEF	3	/* undefined! */
#define FMT_DEFAULT	FMT_INTEL

/* external refs for function converters */
extern WRHEXFUNC	wr_intel;
extern WRHEXFUNC	wr_intel86;
extern WRHEXFUNC	wr_intel32;
extern RDHEXFUNC	rd_intel;
extern SCANHEXFUNC	scan_intel;

#endif /* __hex_h */
