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

/*
 * BUGS:
 *	Start addresses are ignored on input and omitted on output,
 *	due to lack of documentation. Entry address bounds are checked
 *	by the writers, however.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "etools.h"
#include "hex.h"

#define CPL 16			/* data chars per line when writing */

/* byte offsets of record fields (raw binary form) */
#define B_BCOUNT	0
#define B_ADDR		1
#define B_RTYPE		3
#define B_DATA		4

/* byte offsets of record fields (hex char form with leading ':') */
#define H_BCOUNT	1
#define H_ADDR		3
#define H_RTYPE		7
#define H_DATA		9
#define LINEBUFLEN	512	/* length of read buffers */
#define ADDRMASK	0xffff	/* address mask for intel86 and intel32 */

/* record types */
#define REC_DATA	0
#define REC_EOF		1
#define REC_EXT		2
#define REC_START	3
#define REC_EXTLIN	4
#define REC_STARTLIN	5

#define RDERR(a) free(buf); ERR((a))


/*---------------------------------------------------------------*/
int rd_intel(FILE *in, FILE *out, int ignoresum, ULONG *minaddr, ULONG *entry)
{
    UCHAR	linebuf[LINEBUFLEN];
    UCHAR	binbuf[LINEBUFLEN/2];
    UCHAR	*buf;
    UCHAR	checksum;
    ULONG	Lminaddr, Lmaxaddr, Lentry;
    ULONG	addr, base, linaddr, bufsize;
    int		linelen, i;
    
    linaddr = 0;

    /* determine size and address range of data */
    if(scan_intel(in, NULL, &Lminaddr, &Lmaxaddr, &Lentry))
	return hex_errno;
    rewind(in);
    

    /* allocate buffer for data */
    bufsize = (Lmaxaddr - Lminaddr) + 1;
    buf = (UCHAR *)malloc(bufsize);

    if(!buf) {
	ERR(H_ERR_IO);
    }

    while(fgets((char *)linebuf, LINEBUFLEN-1, in))
    {
	/* ignore short lines */
	linelen = strlen((char *)linebuf);
	if(linelen < H_DATA)
	    continue;
	
	/* ignore lines without leading colon */
	if(linebuf[0] != ':')
	    continue;

	/* convert hex to chars */
	for(i=0; i < (linelen-1) >> 1; i++) {
	    binbuf[i] = H2C(&linebuf[(i << 1) + 1]);
	}

	/* check line length:
	   line length should be at least (H_DATA bytes for header)
	   + (2 * number of bytes specified in byte count field)
	   + (2 bytes for checksum) + (1 byte for newline) */
	if (linelen < (H_DATA + 3 + binbuf[B_BCOUNT])) {
	    RDERR(H_ERR_BADHEX);
	}

	/* compute checksum */
	if (!ignoresum) {
	    for(checksum=0, i=0; i < (B_DATA + binbuf[B_BCOUNT] + 1); i++)
		checksum += binbuf[i];
	    if(checksum) {
		RDERR(H_ERR_BADSUM);
	    }
	}
	
	/* process the line */
	switch(binbuf[B_RTYPE]) {

	  case REC_DATA:	/* data record */
	    addr = ((binbuf[B_ADDR] << 8) | binbuf[B_ADDR + 1]) \
		+ base + linaddr;
	    for(i=0; i < binbuf[B_BCOUNT]; i++)
		buf[(addr-Lminaddr) + i] = binbuf[B_DATA + i];
	    break;

	  case REC_EOF:		/* end of file record */
	    break;

	  case REC_EXT:		/* extended address record */
	    base = (binbuf[B_DATA] << 12) | (binbuf[B_DATA+1] << 4);
	    break;

	  case REC_START:	/* start record */
	    /* not documented in Data I/O manual... just
	       ignore this record until proper spec is found */
	    break;

	  case REC_EXTLIN:	/* extended linear address record */
	    linaddr = (binbuf[B_DATA] << 24) | (binbuf[B_DATA+1] << 16);
	    break;

	  case REC_STARTLIN:	/* start linear address record */
	    /* not documented in Data I/O manual... just
	       ignore this record until proper spec is found */
	    break;

	  default:		/* error */
	    RDERR(H_ERR_RECTYPE);
	    break;
	}

	/* if record was an end of file record, stop reading */
	if(binbuf[B_RTYPE] == REC_EOF)
	    break;
    }

    /* check for I/O error */
    if(ferror(in)) {
	RDERR(H_ERR_IO);
    }

    if(fwrite(buf, 1, bufsize, out) != bufsize) {
	RDERR(H_ERR_IO);
    }

    free(buf);

    /* copy local scan result variables to outside world, checking
       that pointers are not NULL first: */
    if (minaddr)
	*minaddr = Lminaddr;

    if (entry)
	*entry = Lentry;

    return H_ERR_NONE;
}


/*---------------------------------------------------------------*/
int wr_intel(FILE *in, FILE *out, ULONG base, ULONG entry)
{
    UCHAR	inbuf[CPL+5];
    UCHAR	outbuf[(2*CPL)+12];
    ULONG	addr;
    int		linelen, i;

    if(entry > MAXADDR_INTEL) {
	/* entry won't fit into its field */
	ERR(H_ERR_ENTRY);
    }

    addr = base;
    outbuf[0] = ':';
    inbuf[B_RTYPE] = REC_DATA;

    while(!feof(in)) {

	if((linelen = fread(&inbuf[B_DATA],1,CPL,in)) > 0) {

	    if(addr > (MAXADDR_INTEL - (ULONG)(linelen-1))) {
		/* address of some byte is too large */
		ERR(H_ERR_ADDR);
	    }

	    /* byte count */
	    inbuf[B_BCOUNT] = (char)linelen;

	    /* address */
	    inbuf[B_ADDR]   = (addr>>8) & 0xff;
	    inbuf[B_ADDR+1] = addr & 0xff;

	    /* calc checksum and convert to hex */
	    for(inbuf[B_DATA+linelen]=0, i=0; i < B_DATA + linelen; i++) {
		inbuf[B_DATA+linelen] += inbuf[i];
		outbuf[(i<<1)+1] = C2H_H(inbuf[i]);
		outbuf[(i<<1)+2] = C2H_L(inbuf[i]);
	    }
	    inbuf[B_DATA+linelen] = ~inbuf[B_DATA+linelen] + 1; /* 2's compl */
	    outbuf[H_DATA + (linelen<<1)]     = C2H_H(inbuf[B_DATA+linelen]);
	    outbuf[H_DATA + (linelen<<1) + 1] = C2H_L(inbuf[B_DATA+linelen]);

	    /* terminate with newline */
	    outbuf[H_DATA + (linelen<<1) + 2] = '\n';

	    /* write the line */
	    if(fwrite(outbuf,1,H_DATA + (linelen<<1) + 3,out) == 0) {
		ERR(H_ERR_IO);
	    }
	    addr += linelen;
	}
	else if(ferror(in)) {
	    /* error while reading */
	    ERR(H_ERR_IO);
	}
    }

    /* write end record */

    /* byte count */
    inbuf[B_BCOUNT] = 0;

    /* address */
    inbuf[B_ADDR]   = 0;
    inbuf[B_ADDR+1] = 0;

    /* record type */
    inbuf[B_RTYPE] = REC_EOF;

    /* calc checksum and convert to hex */
    for(inbuf[B_DATA]=0, i=0; i < B_DATA; i++) {
	inbuf[B_DATA] += inbuf[i];
	outbuf[(i<<1)+1] = C2H_H(inbuf[i]);
	outbuf[(i<<1)+2] = C2H_L(inbuf[i]);
    }
    inbuf[B_DATA] = ~inbuf[B_DATA] + 1; /* 2's compl */
    outbuf[H_DATA]     = C2H_H(inbuf[B_DATA]);
    outbuf[H_DATA + 1] = C2H_L(inbuf[B_DATA]);

    /* terminate with newline */
    outbuf[H_DATA + 2] = '\n';

    /* write the line */
    if(fwrite(outbuf,1,H_DATA + 3,out) == 0) {
	ERR(H_ERR_IO);
    }

    return H_ERR_NONE;
}


/*---------------------------------------------------------------*/
int wr_intel86(FILE *in, FILE *out, ULONG base, ULONG entry)
{
    UCHAR	inbuf[CPL+5];
    UCHAR	outbuf[(2*CPL)+12];
    ULONG	addr, rdlen;
    int		linelen, i;
    int		initsegment=TRUE;
    
    /* base and entry are absolute addresses. addr is the absolute
       address of a given byte, and must be masked for various fields */
    if(entry > MAXADDR_INTEL86) {
	/* entry won't fit into its field */
	ERR(H_ERR_ENTRY);
    }
    addr = base;
    outbuf[0] = ':';

    while(!feof(in)) {

	/* compute # of bytes left in this segment, and read
	   min(bytes left, CPL) bytes for current data record */
	rdlen = (ADDRMASK + 1) - (addr & ADDRMASK);

	if((rdlen == ADDRMASK + 1) || initsegment) {
	    /* time for a new extended address record... */

	    /* segment address specifies bits 4-19 of the address,
	       but only bits 16-19 are printed here by this code because
	       the data record address fields can specify the other bits */
	    initsegment = FALSE;
	    linelen = 2;
	    inbuf[B_BCOUNT] = linelen;
	    inbuf[B_ADDR] = 0;
	    inbuf[B_ADDR+1] = 0;
	    inbuf[B_RTYPE] = REC_EXT;
	    inbuf[B_DATA] = ((addr & ~ADDRMASK) >> 12) & 0xff;
	    inbuf[B_DATA+1] = 0;

	    /* calc checksum and convert to hex */
	    for(inbuf[B_DATA+linelen]=0, i=0; i < B_DATA + linelen; i++) {
		inbuf[B_DATA+linelen] += inbuf[i];
		outbuf[(i<<1)+1] = C2H_H(inbuf[i]);
		outbuf[(i<<1)+2] = C2H_L(inbuf[i]);
	    }
	    inbuf[B_DATA+linelen] = ~inbuf[B_DATA+linelen] + 1; /* 2's compl */
	    outbuf[H_DATA + (linelen<<1)]     = C2H_H(inbuf[B_DATA+linelen]);
	    outbuf[H_DATA + (linelen<<1) + 1] = C2H_L(inbuf[B_DATA+linelen]);

	    /* terminate with newline */
	    outbuf[H_DATA + (linelen<<1) + 2] = '\n';

	    /* write the line */
	    if(fwrite(outbuf,1,H_DATA + (linelen<<1) + 3,out) == 0) {
		ERR(H_ERR_IO);
	    }
	}

	rdlen = MIN(CPL,rdlen);

	if((linelen = fread(&inbuf[B_DATA],1,rdlen,in)) > 0) {

	    if(addr > (MAXADDR_INTEL86 - (ULONG)(linelen-1))) {
		/* address of some byte is too large */
		ERR(H_ERR_ADDR);
	    }

	    inbuf[B_RTYPE] = REC_DATA;

	    /* byte count */
	    inbuf[B_BCOUNT] = (char)linelen;

	    /* address */
	    inbuf[B_ADDR]   = (addr>>8) & 0xff;
	    inbuf[B_ADDR+1] = addr & 0xff;

	    /* calc checksum and convert to hex */
	    for(inbuf[B_DATA+linelen]=0, i=0; i < B_DATA + linelen; i++) {
		inbuf[B_DATA+linelen] += inbuf[i];
		outbuf[(i<<1)+1] = C2H_H(inbuf[i]);
		outbuf[(i<<1)+2] = C2H_L(inbuf[i]);
	    }
	    inbuf[B_DATA+linelen] = ~inbuf[B_DATA+linelen] + 1; /* 2's compl */
	    outbuf[H_DATA + (linelen<<1)]     = C2H_H(inbuf[B_DATA+linelen]);
	    outbuf[H_DATA + (linelen<<1) + 1] = C2H_L(inbuf[B_DATA+linelen]);

	    /* terminate with newline */
	    outbuf[H_DATA + (linelen<<1) + 2] = '\n';

	    /* write the line */
	    if(fwrite(outbuf,1,H_DATA + (linelen<<1) + 3,out) == 0) {
		ERR(H_ERR_IO);
	    }
	    addr += linelen;
	}
	else if(ferror(in)) {
	    /* error while reading */
	    ERR(H_ERR_IO);
	}
    }

    /* write end record */

    /* byte count */
    inbuf[B_BCOUNT] = 0;

    /* address */
    inbuf[B_ADDR]   = 0;
    inbuf[B_ADDR+1] = 0;

    /* record type */
    inbuf[B_RTYPE] = REC_EOF;

    /* calc checksum and convert to hex */
    for(inbuf[B_DATA]=0, i=0; i < B_DATA; i++) {
	inbuf[B_DATA] += inbuf[i];
	outbuf[(i<<1)+1] = C2H_H(inbuf[i]);
	outbuf[(i<<1)+2] = C2H_L(inbuf[i]);
    }
    inbuf[B_DATA] = ~inbuf[B_DATA] + 1; /* 2's compl */
    outbuf[H_DATA]     = C2H_H(inbuf[B_DATA]);
    outbuf[H_DATA + 1] = C2H_L(inbuf[B_DATA]);

    /* terminate with newline */
    outbuf[H_DATA + 2] = '\n';

    /* write the line */
    if(fwrite(outbuf,1,H_DATA + 3,out) == 0) {
	ERR(H_ERR_IO);
    }

    return H_ERR_NONE;
}


/*---------------------------------------------------------------*/
int wr_intel32(FILE *in, FILE *out, ULONG base, ULONG entry)
{
    UCHAR	inbuf[CPL+5];
    UCHAR	outbuf[(2*CPL)+12];
    ULONG	addr, rdlen;
    int		linelen, i;
    int		initsegment=TRUE;
    
    /* base and entry are absolute addresses. addr is the absolute
       address of a given byte, and must be masked for various fields */
    if(entry > MAXADDR_INTEL32) {
	/* entry won't fit into its field */
	ERR(H_ERR_ENTRY);
    }
    addr = base;
    outbuf[0] = ':';

    while(!feof(in)) {

	/* compute # of bytes left in this segment, and read
	   min(bytes left, CPL) bytes for current data record */
	rdlen = (ADDRMASK + 1) - (addr & ADDRMASK);

	if((rdlen == ADDRMASK + 1) || initsegment) {
	    /* time for a new extended linear address record */

	    /* some programmers output an extended address segment
	       record and an extended linear address record, but this
	       code just uses an extended linear address record for
	       bits 16-31, and does not output extended segment records */
	    initsegment = FALSE;
	    linelen = 2;
	    inbuf[B_BCOUNT] = linelen;
	    inbuf[B_ADDR] = 0;
	    inbuf[B_ADDR+1] = 0;
	    inbuf[B_RTYPE] = REC_EXTLIN;
	    inbuf[B_DATA] = ((addr & ~ADDRMASK) >> 24) & 0xff;
	    inbuf[B_DATA+1] = ((addr & ~ADDRMASK) >> 16) & 0xff;

	    /* calc checksum and convert to hex */
	    for(inbuf[B_DATA+linelen]=0, i=0; i < B_DATA + linelen; i++) {
		inbuf[B_DATA+linelen] += inbuf[i];
		outbuf[(i<<1)+1] = C2H_H(inbuf[i]);
		outbuf[(i<<1)+2] = C2H_L(inbuf[i]);
	    }
	    inbuf[B_DATA+linelen] = ~inbuf[B_DATA+linelen] + 1; /* 2's compl */
	    outbuf[H_DATA + (linelen<<1)]     = C2H_H(inbuf[B_DATA+linelen]);
	    outbuf[H_DATA + (linelen<<1) + 1] = C2H_L(inbuf[B_DATA+linelen]);

	    /* terminate with newline */
	    outbuf[H_DATA + (linelen<<1) + 2] = '\n';

	    /* write the line */
	    if(fwrite(outbuf,1,H_DATA + (linelen<<1) + 3,out) == 0) {
		ERR(H_ERR_IO);
	    }
	}

	rdlen = MIN(CPL,rdlen);

	if((linelen = fread(&inbuf[B_DATA],1,rdlen,in)) > 0) {

	    if(addr > (MAXADDR_INTEL32 - (ULONG)(linelen-1))) {
		/* address of some byte is too large */
		ERR(H_ERR_ADDR);
	    }

	    inbuf[B_RTYPE] = REC_DATA;

	    /* byte count */
	    inbuf[B_BCOUNT] = (char)linelen;

	    /* address */
	    inbuf[B_ADDR]   = (addr>>8) & 0xff;
	    inbuf[B_ADDR+1] = addr & 0xff;

	    /* calc checksum and convert to hex */
	    for(inbuf[B_DATA+linelen]=0, i=0; i < B_DATA + linelen; i++) {
		inbuf[B_DATA+linelen] += inbuf[i];
		outbuf[(i<<1)+1] = C2H_H(inbuf[i]);
		outbuf[(i<<1)+2] = C2H_L(inbuf[i]);
	    }
	    inbuf[B_DATA+linelen] = ~inbuf[B_DATA+linelen] + 1; /* 2's compl */
	    outbuf[H_DATA + (linelen<<1)]     = C2H_H(inbuf[B_DATA+linelen]);
	    outbuf[H_DATA + (linelen<<1) + 1] = C2H_L(inbuf[B_DATA+linelen]);

	    /* terminate with newline */
	    outbuf[H_DATA + (linelen<<1) + 2] = '\n';

	    /* write the line */
	    if(fwrite(outbuf,1,H_DATA + (linelen<<1) + 3,out) == 0) {
		ERR(H_ERR_IO);
	    }
	    addr += linelen;
	}

	else if(ferror(in)) {
	    /* error while reading */
	    ERR(H_ERR_IO);
	}
    }

    /* write end record */

    /* byte count */
    inbuf[B_BCOUNT] = 0;

    /* address */
    inbuf[B_ADDR]   = 0;
    inbuf[B_ADDR+1] = 0;

    /* record type */
    inbuf[B_RTYPE] = REC_EOF;

    /* calc checksum and convert to hex */
    for(inbuf[B_DATA]=0, i=0; i < B_DATA; i++) {
	inbuf[B_DATA] += inbuf[i];
	outbuf[(i<<1)+1] = C2H_H(inbuf[i]);
	outbuf[(i<<1)+2] = C2H_L(inbuf[i]);
    }
    inbuf[B_DATA] = ~inbuf[B_DATA] + 1; /* 2's compl */
    outbuf[H_DATA]     = C2H_H(inbuf[B_DATA]);
    outbuf[H_DATA + 1] = C2H_L(inbuf[B_DATA]);

    /* terminate with newline */
    outbuf[H_DATA + 2] = '\n';

    /* write the line */
    if(fwrite(outbuf,1,H_DATA + 3,out) == 0) {
	ERR(H_ERR_IO);
    }

    return H_ERR_NONE;
}


/*---------------------------------------------------------------*/
int scan_intel(FILE *in, ULONG *size, ULONG *minaddr, ULONG *maxaddr, 
	       ULONG *entry)
{
    UCHAR	linebuf[LINEBUFLEN];
    UCHAR	binbuf[LINEBUFLEN/2];
    ULONG	addr, base, linaddr;
    ULONG	Lsize, Lminaddr, Lmaxaddr, Lentry;
    int		linelen, i;

    /* This function does not check for bad hex data checksums or
       incorrect byte count fields.
       Its main purpose is to determine how much memory will be required
       by rd_intel(), which will check the data more carefully.
       The size argument returns the number of data bytes in the file,
       which is not necessarily the same as *maxaddr-*minaddr if the
       file is sparse. */
    
    Lsize = Lmaxaddr = Lentry = base = linaddr = 0;
    Lminaddr = 0xffffffff;

    while(fgets((char *)linebuf, LINEBUFLEN-1, in))
    {
	/* ignore short lines */
	linelen = strlen((char *)linebuf);
	if(linelen < H_DATA)
	    continue;
	
	/* ignore lines without leading colon */
	if(linebuf[0] != ':')
	    continue;

	/* convert hex to chars */
	for(i=0; i < (linelen-1) >> 1; i++) {
	    binbuf[i] = H2C(&linebuf[(i << 1) + 1]);
	}

	/* process the line */
	switch(binbuf[B_RTYPE]) {

	  case REC_DATA:	/* data record */
	    addr = ((binbuf[B_ADDR] << 8) | binbuf[B_ADDR + 1]) \
		+ base + linaddr;
	    Lminaddr = MIN(Lminaddr, addr);
	    Lmaxaddr = MAX(Lmaxaddr, addr + binbuf[B_BCOUNT] - 1);
	    Lsize += binbuf[B_BCOUNT];
	    break;

	  case REC_EOF:		/* end of file record */
	    break;

	  case REC_EXT:		/* extended address record */
	    base = (binbuf[B_DATA] << 12) | (binbuf[B_DATA+1] << 4);
	    break;

	  case REC_START:	/* start record */
	    /* not documented in Data I/O manual... just
	       ignore this record until proper spec is found */
	    break;

	  case REC_EXTLIN:	/* extended linear address record */
	    linaddr = (binbuf[B_DATA] << 24) | (binbuf[B_DATA+1] << 16);
	    break;

	  case REC_STARTLIN:	/* start linear address record */
	    /* not documented in Data I/O manual... just
	       ignore this record until proper spec is found */
	    break;

	  default:		/* error */
	    ERR(H_ERR_RECTYPE);
	    break;
	}

	/* if record was an end of file record, stop reading */
	if(binbuf[B_RTYPE] == REC_EOF)
	    break;
    }

    /* check for I/O error */
    if(ferror(in)) {
	ERR(H_ERR_IO);
    }

    /* copy local scan result variables to outside world, checking
       that pointers are not NULL first: */
    if (size)
	*size = Lsize;

    if (minaddr)
	*minaddr = Lminaddr;

    if (maxaddr)
	*maxaddr = Lmaxaddr;

    if (entry)
	*entry = Lentry;
    
    return H_ERR_NONE;
}
