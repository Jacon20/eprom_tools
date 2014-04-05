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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include "etools.h"
#include "hex.h"

#define VERSION "version 0.2 (ALPHA) (C) 1995 Mark J. Blair, distributed under GPLv3"

void version(int bin2hex)
{
    if (bin2hex) {
	fprintf(stderr,"bin2hex %s\n", VERSION);
    }
    else {
	fprintf(stderr,"hex2bin %s\n", VERSION);
    }
}


void usage(int bin2hex)
{
    int i;

    version(bin2hex);
    if (bin2hex) {
	fprintf(stderr,"\nUsage:  bin2hex [-f{format}] [-b{base}] "\
		"[-e{entry}] [-] [-q] [{infile} [{outfile}]]\n");
	fprintf(stderr,"        bin2hex -help\n");
	fprintf(stderr,"        bin2hex -?\n");
	fprintf(stderr,"        bin2hex -version\n");
    }
    else {
	fprintf(stderr,"\nUsage:  hex2bin [-f{format}] "\
		"[-i] [-q] [-] [{infile} [{outfile}]]\n");
	fprintf(stderr,"        hex2bin -help\n");
	fprintf(stderr,"        hex2bin -?\n");
	fprintf(stderr,"        bin2hex -version\n");
    }
    fprintf(stderr,"\n    formats supported:\n");
    for(i=0; converters[i].name; i++) {
	fprintf(stderr,"        %-16s %s\n", converters[i].name,
		converters[i].desc);
    }
}


/* this is the entry point for both hex2bin and bin2hex */
int main(int argc, char **argv)
{
    int		format = FMT_DEFAULT;
    int		bin2hex;
    int		i,j;		/* temp vars */
    int		ignoresum = FALSE, argsdone = FALSE, quiet = FALSE;
    ULONG	base = 0, entry = 0;
    FILE	*in = NULL, *out = NULL;
    char	*c;		/* temp char pointer */

    /* decide whether to convert bin to hex or vice versa */
    if (strcmp(argv[0]+strlen(argv[0])-7,"bin2hex") == 0)
	bin2hex = TRUE;

    else if (strcmp(argv[0]+strlen(argv[0])-7,"hex2bin") == 0)
	bin2hex = FALSE;

    else {
	fprintf(stderr,"I must be called bin2hex or " \
		"hex2bin so that I know what to do!\n");
	exit(1);
    }

    /* parse arguments */
    for(i=1; i<argc; i++) {

	if ((argv[i][0] == '-') && !argsdone) {

	    /* argument is a flag */
	    if (strlen(argv[i]) > 1) {

		switch(argv[i][1]) {

		  case 'i':
		    ignoresum = TRUE;
		    break;

		  case 'f':
		    format = FMT_UNDEF;

		    if (strlen(argv[i]) > 2) {

			for(j=0; converters[j].name ; j++) {

			    if (strcmp(converters[j].name,argv[i]+2)==0) {
				format=j;
				break;
			    }
			}
		    }

		    if (format == FMT_UNDEF) {
			fprintf(stderr,
				"Error: Unknown format \"%s\"\n",argv[i]);
			usage(bin2hex);
			exit(1);
		    }

		    break;

		  case 'b':
		    c="M";	/* arbitrary non-NUL character */

		    if (strlen(argv[i]) > 2) {
			base=(ULONG)strtol(argv[i]+2,&c,0);
		    }

		    if (c[0] != '\0') {
			fprintf(stderr,"Error: invalid base address\n");
			usage(bin2hex);
			exit(1);
		    }


		    break;

		  case 'e':
		    c="M";	/* arbitrary non-NUL character */

		    if (strlen(argv[i]) > 2) {
			entry=(ULONG)strtol(argv[i]+2,&c,0);
		    }

		    if (c[0] != '\0') {
			fprintf(stderr,"Error: invalid base address\n");
			usage(bin2hex);
			exit(1);
		    }
		    break;

		  case 'h':
		  case '?':
		    usage(bin2hex);
		    exit(0);
		    break;

		  case 'v':
		    version(bin2hex);
		    exit(0);
		    break;

		  case 'q':
		    quiet = TRUE;
		    break;

		  default:
		    fprintf(stderr,"Error: unknown flag \"%s\"\n",argv[i]);
		    usage(bin2hex);
		    exit(1);
		    break;
		}
	    }

	    else {
		/* argument was "-", so stop parsing flags
		   (this will permit filenames beginning with
		   '-' to be specified; could also just specify them
		   as "./-file" if this feature was not present) */
		argsdone = TRUE;
	    }
	}

	else
	{
	    /* argument is a filename */
	    if (in) {

		if (out) {
		    /* if in and out were already specified,
		       then there should be no more non-flag
		       arguments */
		    fprintf(stderr,"Error: too many arguments\n");
		    usage(bin2hex);
		    exit(1);
		}

		else {
		    out=fopen(argv[i],"w");

		    if (!out) {
			perror(argv[i]);
			exit(1);
		    }
		}
	    }
	    else {
		in=fopen(argv[i],"r");

		if (!in) {
		    perror(argv[i]);
		    exit(1);
		}
	    }
	}
    }

    /* if input file not specified, copy stdin to a temp file
       so that converters can fseek() in it if necessary */
    if (!in) {

	if (!(in=tmpfile())) {
	    perror("Error opening temporary file");
	    exit(1);
	}

	if (!quiet) {
	    fprintf(stderr,"(reading from stdin)\n");
	}

	if (fcat(stdin,in)) {
	    perror("Error writing to temporary file");
	    exit(1);
	}
	rewind(in);
    }

    /* if output file not specified, use stdout */
    if (!out)
	out=stdout;

    if (bin2hex) {
	/* convert bin to hex */
	if (converters[format].wr_hex(in,out,base,entry)) {
	    hex_perror("Error converting binary to hex");
	    exit(1);
	}
    }

    else {
	/* convert hex to bin */
	/*** do magic here ***/
	if (converters[format].rd_hex(in,out,ignoresum,&base,&entry)) {
	    hex_perror("Error converting hex to binary");
	    exit(1);
	}
	fflush(out);

	if (!quiet) {
	    fprintf(stderr,"base: 0x%08lX entry: 0x%08lX\n", base, entry);
	}
    }

    exit(0);
}
