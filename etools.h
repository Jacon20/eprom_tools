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

#ifndef __etools_h
#define __etools_h

typedef unsigned long int ULONG;
typedef unsigned char UCHAR;
typedef signed char SCHAR;

#ifdef sun
#include <stdlib.h>
#include <stdio.h>
extern size_t fread(void *,size_t, size_t, FILE *);
extern size_t fwrite(const void *, size_t, size_t, FILE *);
extern int printf(char *, ...);
extern int fprintf(FILE *,char *, ...);
extern void perror(char *);
extern long strtol(char *, char **, int);
extern void rewind(FILE *);
extern int fflush(FILE *);
#endif /* sun */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif /* MIN */

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif /* MAX */

#endif /* __etools_h */
