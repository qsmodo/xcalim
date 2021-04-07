#ifndef lint
static char    *copyright = "@(#)Copyright 1989,1990,1993 Peter Collinson, Hillside Systems";
#endif				/* lint */
/***

* program name:
	ptime.c
* function:
	diagnostic program for strftime.
	When given a number of strings, eg
		ptime "%x X"
	will invoke the strftime routine to print the result.
* libraries used:
	standard
* compile time parameters:
	cc -o ptime -O ptime.c
* history:
	Written November 1993
	Peter Collinson
	Hillside Systems
* (C) Copyright: 1993 Hillside Systems/Peter Collinson
	
***/
#include <stdio.h>
#include <unistd.h>
#include <time.h>

char	buf[512];

main(argc, argv)
	int	argc;
	char	**argv;
{
	time_t		ti;
	struct tm	*tm;
	time(&ti);
	tm = localtime(&ti);

	while (--argc) {
		argv++;
		strftime(buf, sizeof buf, *argv, tm);
		printf("%s - %s\n", *argv, buf);
	}
	exit(0);
}
