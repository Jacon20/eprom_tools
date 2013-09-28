#ifndef lint
static char sccsid[]="$Id: //miscsw/eprom/libhex.c#1 $";
#endif

/*
 * eprom_tools: A collection of utilities for use with EPROM programmers
 *
 * Copyright (C) 1995 Mark J. Blair
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "etools.h"
#include "hex.h"

#define FCATBUFLEN 1024

char _hex2nybble_[] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  1,  2,  3,  4,  5,  6,  7,
    8,  9,  0,  0,  0,  0,  0,  0,
    0, 10, 11, 12, 13, 14, 15,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0, 10, 11, 12, 13, 14, 15,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0
};


char _nybble2hex_[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

int hex_errno=0;
const int hex_nerr = 8;
const char *hex_errlist[] = {
    "No error",
    "Address too large for field",
    "I/O error",
    "Invalid hex data",
    "Unknown record type",
    "Bad checksum",
    "Entry address too large for field"
};

void hex_perror(char *s)
{
    if(s) {
	if(strlen(s)>0) {
	    if(hex_errno >= hex_nerr)
		fprintf(stderr,"%s: Error %d\n",s,hex_errno);
	    else if(hex_errno == H_ERR_IO)
		perror(s);
	    else
		fprintf(stderr,"%s: %s\n",s,hex_errlist[hex_errno]);
	    return;
	}
    }
    if(hex_errno >= hex_nerr)
	fprintf(stderr,"Error %d\n",hex_errno);
    else if(hex_errno == H_ERR_IO)
	perror(s);
    else
	fprintf(stderr,"%s\n",hex_errlist[hex_errno]);
}


int fcat(FILE *in, FILE *out)
{
    int len;
    char buf[FCATBUFLEN];

    while((len=fread(buf,1,FCATBUFLEN,in)))
	fwrite(buf,1,len,out);
    if(ferror(in))
	return errno;
    if(ferror(out))
	return errno;
    return 0;
}


CONVSTRUCT converters[] = {
    {"intel","Intel Intellec 8/MDS",
	 MAXADDR_INTEL,rd_intel,wr_intel,scan_intel,"",0,0},
    {"intel86","Intel MCS-86 Hexadecimal Object",
	 MAXADDR_INTEL86,rd_intel,wr_intel86,scan_intel,":",1,0},
    {"intel32","Intel Hex-32",
	 MAXADDR_INTEL32,rd_intel,wr_intel32,scan_intel,":",1,0},
    {NULL,NULL,0,NULL,NULL,NULL,NULL,0,0}
};
