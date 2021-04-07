#ifndef lint
static char    *copyright = "@(#)Copyright 1993 Peter Collinson, Hillside Systems";
#endif                          /* lint */
#include "version.h"
/***
 
* program name:
        xcalev.c
* function:
	Load's xcalim data files from a `calendar' like data file
	called regular stored in the standard Calendar directory
* switches:
	-v	print version and exit
	-f	name of regular file
	-d dir	Use "dir" instead of Calendar
	-x	be compatible with the X calendar program
		(ie flat structure in the Calendar directory)
	-r	remove entries from the files
	followed by an optional year

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

static char	*progname = "xcalev";	/* name of program */
static char	*regular = "regular";	/* name of regular file */
static char	*calendar = "Calendar";	/* name of calendar directory */
static int	compat;			/* if non-zero use calendar compatible files */
static int	expunge;		/* remove entries rather than add them */

static char	*calfmt;		/* printf format of a calendar file */
static char	*caldir;		/* name of a calendar directory */

static char	*months[]	= {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
				   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static int days_in_months[] = { 
    31, 29, 31, 30,
    31, 30, 31, 31,
    30, 31, 30, 31,
};

static int	year;			/* year to do the work for */

/*
 * various strings
 */
static char	*memerr = "No more memory\n";
static char	*usage = "Usage: xcalev [-f srcfile] [-d dir][-x][-r] [year]\n";

/*
 *	routines
 */
static void filenames();
static void lineprocess();
static int monthmatch();
static void fileupdate();
static char *connect_file();
static void disconnect_file();
static char *delstr();
static void rewrite_file();
static char *Strdup();
static void fatal();
static void error();
static char *readbfile();

main(argc, argv)
	int	argc;
	char	*argv[];
{
	register int	c;
	extern char 	*optarg;
	extern int	optind;
	FILE		*fin;
	char		line[BUFSIZ];

	while ((c = getopt(argc, argv, "d:f:crv")) != EOF) {

		switch (c) {
		case 'd':
			calendar = optarg;
			break;
		case 'f':
			regular = optarg;
			break;
		case 'x':
			compat = 1;
			break;
		case 'r':
			expunge = 1;
			break;
		case 'v':
			fprintf(stderr, "%s\n", version);
			exit(0);
		default:
			fatal(usage);
		}
	}
	if (optind < argc) {
		year = atoi(argv[optind]);
		if (year < 1993 && year > 2100)
			fatal("Year %s should be > 1993\n", argv[optind]);
	}

	filenames();

	/*
	 * open the regular file
	 */
	if ((fin = fopen(regular, "r")) == NULL) {
		fatal("Cannot open: %s\n", regular);
	}
	while (fgets(line, sizeof line, fin) != NULL) {
		lineprocess(line);
	}
	fclose(fin);
	exit(0);
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

	pw = getpwuid(getuid());
	if (pw == NULL)
		fatal("Cannot get password information\n");
	/*
	 * get filename of Calendar directory
	 */
	if (*calendar != '.' && *calendar != '/') {
		/* need to prepend the home directory */
		(void) sprintf(buf, "%s/%s", pw->pw_dir, calendar);
		calendar = Strdup(buf);
	}
	/*
	 * and the source file
	 */
	if (*regular != '.' && *regular != '/') {
		(void) sprintf(buf, "%s/%s", calendar, regular);
		regular = Strdup(buf);
	}
	/*
	 * get the year sorted
	 */
	if (year == 0) {
		time(&ti);
		tm = localtime(&ti);
		year = tm->tm_year + 1900;
	}
	/*
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
	if (compat) 
		(void) sprintf(buf, "%s/xc%%d%%s%d", calendar, year);
	else	(void) sprintf(buf, "%s/xy%d/xc%%d%%s%d", calendar, year, year);
	calfmt = Strdup(buf);
	if (compat)
		caldir = calendar;
	else {
		(void) sprintf(buf, "%s/xy%d", calendar, year);
		caldir = Strdup(buf);
	}

	/*
	 * See if we have a calendar directory
	 */
	if (access(calendar, F_OK) < 0) {
		printf("%s does not exist, creating it\n", calendar);
		if (mkdir(calendar, 0755) < 0) {
			perror("mkdir");
			fatal("Cannot create: %s\n", calendar);
		}
	}
	/*
	 * and an annual one
	 */
	if (access(caldir, F_OK) < 0) {
		printf("%s does not exist, creating it\n", caldir);
		if (mkdir(caldir, 0755) < 0) {
			perror("mkdir");
			fatal("Cannot create: %s\n", caldir);
		}
	}		
}

/*
 *	Process a single line from the regular file
 *	format is
 *	Month day text
 *	or
 *	day Month text
 *	Month is always a text string
 *	day can be a range with no spaces
 *	eg	5-11
 *	Lines starting with # are ignored
 */
static void
lineprocess(line)
	char	*line;
{
	char	 *p1, *p2, *rest;
	char	*ds;
	char	*ms;
	int	day, dayend, i, mon;
	static int lineno;

	lineno++;
	/* comment */
	if (*line == '#')
		return;
	
	/* parse line into three sections */
	p1 = strtok(line, " \t");
	if (p1 == NULL) {
		error("Cannot find month or day on line %d of %s\n", lineno,
			regular);
		return;
	}
	p2 = strtok(NULL, " \t");
	if (p2 == NULL) {
		error("Cannot find month or day on line %d of %s\n",
		      lineno, regular);
		return;
	}
	rest = strtok(NULL, "\n");
	if (rest == NULL) {
		error("No associated text with date on line %d of %s\n", 
		      lineno, regular);
		return;
	}
	/* is p1 or p2 the day? */
	if (isdigit(*p1)) {
		ds = p1;
		ms = p2;
	} else {
		ds = p2;
		ms = p1;
	}
	/* see if we have a range */
	p1 = strchr(ds, '-');
	if (p1) {
		*p1++ = '\0';
		day = atoi(ds);
		dayend = atoi(p1);
		if (dayend < day) {
			error("Illegal day range: %d should be <= than %d\n", day, dayend);
			return;
		}	
	} else {
		day = dayend = atoi(ds);
	}
	mon = monthmatch(ms);
	if (mon == -1) {
		if (isdigit(*ms))
			error("Months must be given as names, line %d of %s\n",
				lineno, regular);
		else
			error("Cannot recognise %s as a month name, line %d of %s\n",
				ms, lineno, regular);
		return;
	}
	if (day < 1 || day > days_in_months[mon]) {
		error("%s does not have a day numbered %d, line %d of %s\n",
			months[mon], day, lineno, regular);
		return;
	}
	if (dayend < 1 || dayend > days_in_months[mon]) {
		error("%s does not have a day numbered %d, line %d of %s\n",
			months[mon], dayend, lineno, regular);
		return;
	}

	/* lose leading space before the text */
	while (isspace(*rest))
		rest++;
	if (*rest == '\0')
		return;
	for (i = day; i <= dayend; i++)
		fileupdate(i, mon, rest);
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
 * update a file from the data
 */
static void
fileupdate(day, mon, txt)
	int	day;
	int	mon;
	char	*txt;
{
	char	filename[256];
	char	*contents;
	char	*substr;
	int	flen;
	int	addnl = 0;
	FILE	*newf;

	(void) sprintf(filename, calfmt, day, months[mon]);

	if (access(filename, F_OK) == 0) {
		/* file exists */
		/* connect in memory and look for the text string */
		contents = connect_file(filename, &flen);
		if (flen) {
			substr = strstr(contents, txt);
			if (substr) {
				/* message already exists */
				if (expunge) {
					/* 
					 * we need to delete the string and
					 * rewrite the file
					 */
					contents = delstr(contents, flen, txt, substr);
					rewrite_file(filename, contents);
					/* all done - exit */
				} 
				/* adding to file */
				/* but already there - so exit */
				disconnect_file(contents, flen);
				return;
			}
			/* add the string at the end of the file */
			/* check if we need to add a leading newline */
			if (contents[flen-1] != '\n')
				addnl = 1;
			disconnect_file(contents, flen);
		}
	}
	if (expunge)
		return;
	newf = fopen(filename, "a");
	if (newf == NULL) {
		error("Cannot open %s for appending\n", filename);
		return;
	}
	if (addnl)
		putc('\n', newf);
	fputs(txt, newf);
	fclose(newf);
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
                error("Cannot open calendar file: %s\n", filename);
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
                fatal("Cannot read %s into memory\n", filename);
 
        close(fd);      /* we have it now */

	fibase[statb.st_size] = '\0';	/* add a terminating NULL */
        *bytes = statb.st_size;
        return(fibase);
}

static void
disconnect_file(base, len)
	char	*base;
	int	len;
{
	free(base);
}

/*
 * delete a text string from a file
 */
static char *
delstr(contents, len, txt, posn)
	char	*contents;
	int	len;
	char	*txt;
	char	*posn;
{
	int	len_of_txt;
	int	len_to_move;
	register char *s, *d;
	
	len_of_txt = strlen(txt);
	if (*(posn + len_of_txt) == '\n')
		len_of_txt++;
	len_to_move = strlen(posn) - len_of_txt;
	/* do this by steam */
	d = posn;
	s = posn + len_of_txt;
	if (*s == '\0') {
		*d = '\0';
		/* eat any newline at the end of the file */
		for(;;) {
			if (d == contents)
				break;
			d--;
			if (*d == '\n')
				*d = '\0';
			else 	
				break;
		}
		return contents;
	}
	while(len_to_move--)
		*d++ = *s++;
	*d = '\0';
	return contents;
}
	
/*
 * rewrite a file from a new contents string
 */
static void
rewrite_file(filename, new)
	char	*filename;
	char	*new;
{
	int	len;
	int	fd;
	
	unlink(filename);
	if (*new == '\0') {
		/* nothing there - delete the file */
		return;
	}
	len = strlen(new);
	if ((fd = creat(filename, 0666)) < 0) {
		error("Cannot create: %s\n", filename);
		return;
	}
	if (write(fd, new, len) != len)
		error("Problem writing %s\n", filename);
	close(fd);
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
	
	base = malloc(len+1);
	if (base == NULL)
		return ((char *)-1);
	if (read(fd, base, len) != len)
		return ((char *)-1);
	return base;
}
