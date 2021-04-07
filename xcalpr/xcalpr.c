#ifndef lint
static char    *copyright = "@(#)Copyright 1993 Peter Collinson, Hillside Systems";
#endif                          /* lint */
#include "version.h"
/***
 
* program name:
        xcalpr.c
* function:
	Prints xcalim calendar entries
* switches:
	-v	print version number, then exit
	-f	name of output file
	-d dir	Use "dir" instead of Calendar
	-c	be compatible with the X calendar program
		(ie flat structure in the Calendar directory)
	-u user	Look at the calendar for that user
	
		followed by a date spec.
		If no date spec then do this week
		If date spec is just a number of month names, then
			print those months
			If month is `next' do the next month
		If the months are followed by a year, then do those months
			in that year
		If the argument is just a year then do that year.
		
* libraries used:
        standard
* compile time parameters:
        standard
* history:
        Written October 1993
        Peter Collinson
        Hillside Systems
* (C) Copyright: 1993 Hillside Systems/Peter Collinson
 
        Permission to use, copy, modify, and distribute this software
        and its documentation for any purpose is hereby granted to
        anyone, provided that the above copyright notice appear
        in all copies and that both that copyright notice and this
        permission notice appear in supporting documentation, and that
        the name of Peter Collinson not be used in advertising or
        publicity pertaining to distribution of the software without
        specific, written prior permission.  Hillside Systems makes no
        representations about the suitability of this software for any
        purpose.  It is provided "as is" without express or implied
        warranty.
 
        Peter Collinson DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
        SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
        AND FITNESS, IN NO EVENT SHALL Peter Collinson BE LIABLE FOR
        ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
        WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
        WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
        ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
        PERFORMANCE OF THIS SOFTWARE.
 
***/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#if defined(NeXT)
#include <sys/fcntl.h>
#endif
#include <sys/stat.h>

static char	*progname = "xcalpr";	/* name of program */
static char	*calendar = "Calendar";	/* name of calendar directory */
static int	compat;			/* if non-zero use calendar */
					/* compatible files */
static int	calcompat;		/* write calendar format output */
static char	*calfmt;		/* printf format of a calendar file */
static char	*caldir;		/* name of a calendar directory */
static char	*user;			/* name of a user */

static char	*months[]	= {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
				   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static char	*daynames[]	= {"Sun", "Mon", "Tue", "Wed",
				   "Thu", "Fri", "Sat"};

static char	*dayfiles[7];		/* connected contents of any dayfiles */

static FILE	*fout;
	
static int mon[] = { 
    31, 28, 31, 30,
    31, 30, 31, 31,
    30, 31, 30, 31,
};

/*
 * various strings
 */
static char	*memerr = "No more memory\n";
static char	*usage = "Usage: xcalpr [-d dir][-x][-c][-u user][-f file] [month list]\n";

/*
 * Routines
 */
static void	thisweek();
static void	showmonth();
static void	showyear();
static void	filenames();
static void	connectdaily();
static void	yearformat();
static void	outputday();
static void	outputdaily();
static char 	*connect_file();
static int	monthmatch();
static char 	*Strdup();
static void	fatal();
static void	error();
static char	*readbfile();

main(argc, argv)
	int	argc;
	char	*argv[];
{
	register int	c;
	extern char 	*optarg;
	extern int	optind;
	int		i;
	char		**av;
	int		ac;
	int		yr;
	fout = stdout;

	while ((c = getopt(argc, argv, "d:u:f:cxv")) != EOF) {

		switch (c) {
		case 'c':
			calcompat = 1;
			break;
		case 'd':
			calendar = optarg;
			break;
		case 'f':
			fout = fopen(optarg, "w");
			if (fout == NULL)
				fatal("Cannot open %s for writing\n", optarg);
			break;
		case 'u':
			user = optarg;
			break;
		case 'v':
			fprintf(stderr, "%s\n", version);
			exit(0);
		case 'x':
			compat = 1;
			break;
		default:
			fatal(usage);
		}
	}
	filenames();
	connectdaily();

	if (optind == argc)
		thisweek();
	else {
		/* 
		 * this is a little complicated
		 * the idea is to allow
		 * xcalcat month
		 * xcalcat month year
		 * xcalcat year
		 * the strategy is to scan forward for a year, storing any
		 * months, if we find a year then output the months for
		 * that year.
		 */
		av = (char **)calloc(argc, sizeof (char *));
		if (av == NULL)
			fatal(memerr);
		ac = 0;
		for (; optind < argc; optind++) {
			if (!isdigit(*argv[optind])) {
				av[ac++] = argv[optind];
			} else {
				yr = atoi(argv[optind]);
				if (ac == 0)
					showyear(yr);
				else {
					for (i = 0; i < ac; i++)
						showmonth(av[i], yr, 0);
					ac = 0;
				}
			}
		}
		if (ac) {
			for (i = 0; i < ac; i++)
				showmonth(av[i], 0, 1);
		}
	}
	exit(0);	
}

/*
 *	Do this week
 */
static void
thisweek()
{	
	time_t		ti;
	struct	tm	*tm;
	int		yr;
	int		i;

	time(&ti);

	for (i = 0; i < 7; i++) {
		tm = localtime(&ti);
		yr = tm->tm_year + 1900;
		yearformat(yr);
		outputday(yr, tm->tm_mon, tm->tm_mday, tm->tm_wday);
		ti += 86400;
	}
}

/*
 *	Do a month
 */
static void
showmonth(mstr, yrval, autoinc)
	char	*mstr;
	int	yrval;
	int	autoinc;
{
	time_t		ti;
	struct	tm	*tm;
	int		yr;
	int		mn;
	int		i;
	int		wday;
	int		startday = 1;

	time(&ti);
	tm = localtime(&ti);
	yr = yrval ? yrval : tm->tm_year + 1900;

	if (strcasecmp(mstr, "next") == 0) {
		mn = tm->tm_mon+1;
		if (mn == 12) {
			mn = 0;
			yr++;
		}
	} else
	if (yrval == 0 && strcasecmp(mstr, "rest") == 0) {
		mn = tm->tm_mon;
		wday = FirstDay(mn, yr);	/* get number of days in month */
		startday = tm->tm_mday+1;
		if (startday > mon[mn]) {
			startday = 1;
			mn++;
			if (mn == 12) {
				mn = 0;
				yr++;

			}
		}
	} else {
		mn = monthmatch(mstr);
		if (mn == -1)
			fatal("Cannot recognise month: %s\n", mstr);
		if (autoinc && mn < tm->tm_mon)
			yr++;
	}

	wday = FirstDay(mn, yr);
	wday = wday + startday - 1;
	wday %= 7;

	yearformat(yr);

	for (i = startday; i <= mon[mn]; i++) {
		outputday(yr, mn, i, wday);
		wday++;
		wday %= 7;
	}
}

/*
 * do a year
 */
static void
showyear(yr)
	int	yr;
{
	int	m;

	for (m = 0; m < 12; m++)
		showmonth(months[m], yr, 0);
}

/*
 * organise filenames
 */
static void
filenames()
{
	struct	passwd	*pw;
	char		buf[256];
	time_t		ti;
	struct	tm	*tm;

	if (user) {
		pw = getpwnam(user);
		if (pw == NULL)
			fatal("Cannot get password information for %s\n", user);
	} else {
		pw = getpwuid(getuid());
		if (pw == NULL)
			fatal("Cannot get password information\n");
	}
	/*
	 * get filename of Calendar directory
	 */
	if (*calendar != '.' && *calendar != '/') {
		/* need to prepend the home directory */
		(void) sprintf(buf, "%s/%s", pw->pw_dir, calendar);
		calendar = Strdup(buf);
	}

	/*
	 * See if we have a calendar directory
	 */
	if (access(calendar, F_OK) < 0)
		fatal("%s does not exist\n", calendar);
	
}

/*
 * Connect any daily files in memory
 */
static void
connectdaily()
{
	int	i;
	char	filename[256];
	int	len;

	for (i = 0; i < 7; i++) {
		(void) sprintf(filename, "%s/xw%s", calendar, daynames[i]);

		dayfiles[i] = connect_file(filename, &len);
	}
}

/*
 * Make file formats from a year
 *
 * xcalendar files are all
 * xc<d><Mon><Year>
 * or
 * xc<dd><Mon><Year>
 *
 * where d or dd is the day (%02d would have been better)
 * <Mon> is a capitalised first three letters of the
 *         English month name. If you need compatibility
 *         don't redefine the short names in the resources
 *         for this program.
 * <Year> is the full numeric year.
 *
 * We will follow this BUT we will also make this program
 * create subdirectories for new years
 *         xy<Year>
 * to speed up file access
 */
static void
yearformat(year)
	int	year;
{
	char	buf[256];

	if (compat) 
		(void) sprintf(buf, "%s/xc%%d%%s%d", calendar, year);
	else	(void) sprintf(buf, "%s/xy%d/xc%%d%%s%d", calendar, year, year);
	if (calfmt)
		free(calfmt);
	calfmt = Strdup(buf);
	if (compat)
		caldir = calendar;
	else {
		(void) sprintf(buf, "%s/xy%d", calendar, year);
		if (caldir)
			free(caldir);
		caldir = Strdup(buf);
	}
}

/*
 * Print data for one day
 */
static void
outputday(year, mn, day, wday)
	int	year;
	int	mn;
	int	day;
	int	wday;
{
	char	filename[256];
	char	line[BUFSIZ];
	char	*p;
	FILE	*fin;
	
	(void) sprintf(filename, calfmt, day, months[mn]);
	if (fin = fopen(filename, "r")) {
		while (fgets(line, sizeof line, fin) != NULL) {
			p = strchr(line, '\n');
			if (p)
				*p = '\0';
			for (p = line; isspace(*p); p++);
			if (*p == '\0')
				continue;
			if (calcompat)
				fprintf(fout, "%s %02d\t%s\n",
				        months[mn], day, p);
			else
				fprintf(fout, "%s %02d %s %4d\t%s\n",
				        daynames[wday], day, months[mn],
					year, p);
		}
		fclose(fin);
	}

	/* check on daily file */
	if (p = dayfiles[wday])
		outputdaily(year, mn, day, wday, p);

}

/*
 * Output a daily file
 */
static void
outputdaily(year, mn, day, wday, txt)
	int	year;
	int	mn;
	int	day;
	int	wday;
	char	*txt;
{
	char	prefix[256];
	char	*sol;
	char	*eol;

	if (calcompat)
		(void) sprintf(prefix, "%s %02d\t", months[mn], day);
	else
		(void) sprintf(prefix, "%s %02d %s %4d\t", daynames[wday],
			       day, months[mn], year);
	
	while (*txt) {
		sol = txt;
		while (*txt != '\n' && *txt != '\0')
			txt++;
		eol = txt;
		while (*sol == ' ' || *sol == '\t')
			sol++;
		if (sol != eol) {
			fputs(prefix, fout);
			while (sol != eol) {
				putc(*sol, fout);
				sol++;
			}
			putc('\n', fout);
		}
		if (*eol == '\n')
			txt++;
	}	
}
	
static char *
connect_file(filename, bytes)
        char    *filename;
        int     *bytes;
{
        int     fd;
        struct  stat statb;
        char    *fibase;
 
        if ((fd = open(filename, 0)) < 0) {
		return NULL;
	}
 
        if (fstat(fd, &statb) < 0)
                fatal("Cannot fstat %s (shouldn't happen)\n", filename);
 
        if (statb.st_size == 0) {
		*bytes = 0;
		return;
	}

	fibase = readbfile(fd, statb.st_size);
        if ((int)fibase == -1)
                fatal("Cannot map %s into memory\n", filename);
 
        close(fd);      /* we have it now */

	fibase[statb.st_size] = '\0';

        *bytes = statb.st_size;
        return(fibase);
}

/*
 * Attempt to recognise a month string
 * return a month (0-11) if OK
 * else -1
 */
static int
monthmatch(str)
	char	*str;
{
	register int i;
	
	for (i = 0; i < 12; i++) {
		if (strncasecmp(str, months[i], 3) == 0)
			return i;
	}
	return -1;
}

/*
 * private strdup this used to call the system strdup()
 * and fail on an error, but it seems that some systems don't a strdup()
 */
static char *
Strdup(str)
	char	*str;
{
	int	len;
	char	*ret;

	len = strlen(str) + 1;
	ret = malloc(len);
	if (ret == NULL)
		fatal(memerr);
	(void) memcpy(ret, str, len);
	return ret;
}

/*
 * fatal routine
 * print an error message and exit
 * should use vprintf I guess
 */
static void
fatal(fmt, a, b, c)
	char	*fmt;
{
	fprintf(stderr, "%s: ", progname);
	fprintf(stderr, fmt, a, b, c);
	exit(1);
}

/*
 * error routine
 */
static void
error(fmt, a, b, c, d)
	char	*fmt;
{
	fprintf(stderr, "%s: ", progname);
	fprintf(stderr, fmt, a, b, c, d);
}

/*
 * malloc some memory and
 * read the file into it
 */
static char *
readbfile(fd, len)
	int	fd;
	int	len;
{
	char	*base;
	
	base = (char *) malloc(len+1);
	if (read(fd, base, len) != len)
		return ((char *)-1);
	return base;
}
