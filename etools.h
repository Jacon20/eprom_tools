/* $Id: //miscsw/eprom/etools.h#1 $	*/

/*
 * eprom_tools: A collection of utilities for use with EPROM programmers
 *
 * Copyright (C) 1995 Mark J. Blair
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
