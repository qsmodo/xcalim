/***
	libXaw.a, libXmu.a libXt.a libX11.a
***/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <X11/Intrinsic.h>
#include <X11/Xos.h>
#include <X11/Xmu/Atoms.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Form.h>
#include "xcalim.h"

char            date_area[BUFSIZ];

/* command line options specific to the application */
static XrmOptionDescRec Options[] = {
    {"-daemon", "daemon", XrmoptionNoArg, (void *) "TRUE"},
	{"-alarmscan", "alarmScan", XrmoptionNoArg, (void *) "TRUE"},
	{"-format", "format", XrmoptionSepArg, NULL},
	{"-stripfmt", "stripFmt", XrmoptionSepArg, NULL},
	{"-editfmt", "editFmt", XrmoptionSepArg, NULL},
	{"-clocktick", "clockTick", XrmoptionSepArg, NULL},
	{"-u", "otherUser", XrmoptionSepArg, NULL},
};

struct resources appResources;

Pixmap          HelpPix;
Pixmap          HelpPressPix;

XtAppContext	appContext;

Atom delWin;

#define offset(field) XtOffset(struct resources *, field)

static XtResource Resources[] = {
	{"otherUser", "OtherUser", XtRString, sizeof(String),
	offset(otheruser), XtRString, NULL},
	{"alarmScan", "AlarmScan", XtRBoolean, sizeof(Boolean),
	offset(alarmScan), XtRString, "False"},
	{"reverseVideo", "ReverseVideo", XtRBoolean, sizeof(Boolean),
	offset(reverseVideo), XtRString, "False"},
	{"justQuit", "JustQuit", XtRBoolean, sizeof(Boolean),
	offset(justQuit), XtRString, "False"},
	{"daemon", "Daemon", XtRBoolean, sizeof(Boolean),
	offset(daemon), XtRString, "False"},
	{"xcalendarCompat", "XcalendarCompat", XtRBoolean, sizeof(Boolean),
	offset(calCompat), XtRString, "False"},
	{"giveHelp", "GiveHelp", XtRBoolean, sizeof(Boolean),
	offset(giveHelp), XtRString, "True"},
	{"helpFromFile", "HelpFromFile", XtRBoolean, sizeof(Boolean),
	offset(helpFromFile), XtRString, "True"},
	{"helpFile", "HelpFile", XtRString, sizeof(String),
	offset(helpfile), XtRString, "/usr/lib/X11/app-defaults/XCalim.help"},
	{"useMemo", "UseMemo", XtRBoolean, sizeof(Boolean),
	offset(useMemo), XtRString, "True"},
	{"memoLeft", "MemoLeft", XtRBoolean, sizeof(Boolean),
	offset(memoLeft), XtRString, "True"},
	{"initialEdit", "InitialEdit", XtRBoolean, sizeof(Boolean),
	offset(initialEdit), XtRString, "False"},
	{"initialMemo", "InitialMemo", XtRBoolean, sizeof(Boolean),
	offset(initialMemo), XtRString, "False"},
	{"format", "Format", XtRString, sizeof(String),
	offset(format), XtRString, "%H:%M:%S"},
	{"stripFmt", "StripFmt", XtRString, sizeof(String),
	offset(stripfmt), XtRString, "%B %Y"},
	{"editFmt", "EditFmt", XtRString, sizeof(String),
	offset(editfmt), XtRString, "%A %d %B %Y"},
	{"clockTick", "ClockTick", XtRInt, sizeof(int),
	offset(clocktick), XtRString, "0"},
	{"markToday", "MarkToday", XtRBoolean, sizeof(Boolean),
	offset(markToday), XtRString, "True"},
	{"todayForeground", "TodayForeground", XtRPixel, sizeof(Pixel),
	offset(today.fg), XtRString, "White"},
	{"todayBackground", "TodayBackground", XtRPixel, sizeof(Pixel),
	offset(today.bg), XtRString, "Black"},
	{"directory", "Directory", XtRString, sizeof(String),
	offset(directory), XtRString, ".xcalim"},
	{"textBufferSize", "TextBufferSize", XtRInt, sizeof(int),
	offset(textbufsz), XtRString, "4004"},
	{"minStripWidth", "MinStripWidth", XtRDimension, sizeof(Dimension),
	offset(minstripwidth), XtRString, "0"},
	/* set to screen size in the code */
	{"maxStripHeight", "MaxStripHeight", XtRDimension, sizeof(Dimension),
	offset(maxstripheight), XtRString, "0"},
	{"january", "January", XtRString, sizeof(String),
	offset(mon[0]), XtRString, "January"},
	{"february", "February", XtRString, sizeof(String),
	offset(mon[1]), XtRString, "February"},
	{"march", "March", XtRString, sizeof(String),
	offset(mon[2]), XtRString, "March"},
	{"april", "April", XtRString, sizeof(String),
	offset(mon[3]), XtRString, "April"},
	{"may", "May", XtRString, sizeof(String),
	offset(mon[4]), XtRString, "May"},
	{"june", "June", XtRString, sizeof(String),
	offset(mon[5]), XtRString, "June"},
	{"july", "July", XtRString, sizeof(String),
	offset(mon[6]), XtRString, "July"},
	{"august", "August", XtRString, sizeof(String),
	offset(mon[7]), XtRString, "August"},
	{"september", "September", XtRString, sizeof(String),
	offset(mon[8]), XtRString, "September"},
	{"october", "October", XtRString, sizeof(String),
	offset(mon[9]), XtRString, "October"},
	{"november", "November", XtRString, sizeof(String),
	offset(mon[10]), XtRString, "November"},
	{"december", "December", XtRString, sizeof(String),
	offset(mon[11]), XtRString, "December"},
	{"jan", "Jan", XtRString, sizeof(String),
	offset(smon[0]), XtRString, "Jan"},
	{"feb", "Feb", XtRString, sizeof(String),
	offset(smon[1]), XtRString, "Feb"},
	{"mar", "Mar", XtRString, sizeof(String),
	offset(smon[2]), XtRString, "Mar"},
	{"apr", "Apr", XtRString, sizeof(String),
	offset(smon[3]), XtRString, "Apr"},
	{"may", "May", XtRString, sizeof(String),
	offset(smon[4]), XtRString, "May"},
	{"jun", "Jun", XtRString, sizeof(String),
	offset(smon[5]), XtRString, "Jun"},
	{"jul", "Jul", XtRString, sizeof(String),
	offset(smon[6]), XtRString, "Jul"},
	{"aug", "Aug", XtRString, sizeof(String),
	offset(smon[7]), XtRString, "Aug"},
	{"sep", "Sep", XtRString, sizeof(String),
	offset(smon[8]), XtRString, "Sep"},
	{"oct", "Oct", XtRString, sizeof(String),
	offset(smon[9]), XtRString, "Oct"},
	{"nov", "Nov", XtRString, sizeof(String),
	offset(smon[10]), XtRString, "Nov"},
	{"dec", "Dec", XtRString, sizeof(String),
	offset(smon[11]), XtRString, "Dec"},
	{"sunday", "Sunday", XtRString, sizeof(String),
	offset(day[0]), XtRString, "Sunday"},
	{"monday", "Monday", XtRString, sizeof(String),
	offset(day[1]), XtRString, "Monday"},
	{"tuesday", "Tuesday", XtRString, sizeof(String),
	offset(day[2]), XtRString, "Tuesday"},
	{"wednesday", "Wednesday", XtRString, sizeof(String),
	offset(day[3]), XtRString, "Wednesday"},
	{"thursday", "Thursday", XtRString, sizeof(String),
	offset(day[4]), XtRString, "Thursday"},
	{"friday", "Friday", XtRString, sizeof(String),
	offset(day[5]), XtRString, "Friday"},
	{"saturday", "Saturday", XtRString, sizeof(String),
	offset(day[6]), XtRString, "Saturday"},
	{"sun", "Sun", XtRString, sizeof(String),
	offset(sday[0]), XtRString, "Sun"},
	{"mon", "Mon", XtRString, sizeof(String),
	offset(sday[1]), XtRString, "Mon"},
	{"tue", "Tue", XtRString, sizeof(String),
	offset(sday[2]), XtRString, "Tue"},
	{"wed", "Wed", XtRString, sizeof(String),
	offset(sday[3]), XtRString, "Wed"},
	{"thu", "Thu", XtRString, sizeof(String),
	offset(sday[4]), XtRString, "Thu"},
	{"fri", "Fri", XtRString, sizeof(String),
	offset(sday[5]), XtRString, "Fri"},
	{"sat", "Sat", XtRString, sizeof(String),
	offset(sday[6]), XtRString, "Sat"},
	{"weekly", "Weekly", XtRString, sizeof(String),
	offset(weekly), XtRString, "Weekly"},
	{"alarms", "Alarms", XtRBoolean, sizeof(Boolean),
	offset(alarms), XtRString, "True"},
	{"execAlarms", "ExecAlarms", XtRBoolean, sizeof(Boolean),
 	offset(execalarms), XtRString, "True"},
	{"alarmWarp", "AlarmWarp", XtRBoolean, sizeof(Boolean),
	offset(alarmWarp), XtRString, "False"},
	{"minAlarmWarp", "MinAlarmWarp", XtRInt, sizeof(int),
	offset(minAlarmWarp), XtRString, "7"},
	{"update", "Update", XtRInt, sizeof(int),
	offset(update), XtRString, "59"},
	{"volume", "Volume", XtRInt, sizeof(int),
	offset(volume), XtRString, "50"},
	{"nbeeps", "Nbeeps", XtRInt, sizeof(int),
	offset(nbeeps), XtRString, "3"},
	{"cmd", "Cmd", XtRString, sizeof(String),
	offset(cmd), XtRString, NULL},
	{"countdown", "Countdown", XtRString, sizeof(String),
	offset(countdown), XtRString, "10,0"},
	{"autoquit", "Autoquit", XtRInt, sizeof(int),
	offset(autoquit), XtRString, "0"},
	{"alarmleft", "Alarmleft", XtRString, sizeof(String),
	offset(alarmleft), XtRString, "%d minutes before..."},
	{"alarmnow", "Alarmnow", XtRString, sizeof(String),
	offset(alarmnow), XtRString, "Time is now..."},
	{"private", "Private", XtRString, sizeof(String),
	offset(private), XtRString, "Private calendar entry"},
	{"already", "Already", XtRString, sizeof(String),
	offset(already), XtRString, "Already editing %d %B %Y"},
	{"alreadyWeekly", "AlreadyWeekly", XtRString, sizeof(String),
	offset(alreadyWeekly), XtRString, "Already editing %A"},
	{"memoFile", "MemoFile", XtRString, sizeof(String),
	offset(memoFile), XtRString, "memo"},
	{"maxDisplayLines", "MaxDisplayLines", XtRInt, sizeof(int),
	offset(maxDisplayLines), XtRString, "5"},
	{"dayWidth", "DayWidth", XtRDimension, sizeof(int),
	offset(squareW), XtRString, "140"},
	{"dayLabelHeight", "DayLabelHeight", XtRDimension, sizeof(int),
	offset(labelH), XtRString, "20"},
	{"dayBoxHeight", "DayBoxHeight", XtRDimension, sizeof(int),
	offset(squareH), XtRString, "80"},
};

static XtCallbackRec callbacks[] = {
	{NULL, NULL},
	{NULL, NULL},
};
#define ClearCallbacks() memset((void *)callbacks, '\0', sizeof (callbacks))

/* * external routines */
extern void   MainHelp();

/* * Forward routines local to this file */
void          AlterTitles();
static  void  ConvDate();
void          MkDate();
static  void  DebugMkDate();
static  int   CycleDays();
static  int   CycleMonths();
static  void  SetUpdateFreq();
static  int   UpdateFreq();
static  void  AdjustHeights();
static  void  PressAButton();
/* *	Create the three components of the date strip */

static XtActionsRec appActions[] = {
    {"Setdate",         SetDate        },
    {"Leave",           AskLeave       },
    {"SetDateAction",   TextCal        },
    {"LoadDateAction",  LoadDateStrip  },
    {"PopDownShell",    PopDownShell   },
    {"PressAButton",    PressAButton   },
    {"PopDownMemo",     CloseMemo      },
    {"CloseHelp",       PopdownHelp    },
    {"StripHelp",       StripHelp      },
    {"DoWeekly",        DoWeekly       },
    {"Nr",              Nr             },
    {"WeekDay",         WeekDay        },
    {"PopUpMemo",       PopUpMemo      },
};

static String quitTranslations = "<Message>WM_PROTOCOLS: Leave()";

Widget          toplevel, parent;

Widget          mHelp;		/* popup help widget */

Date            today;

int		updatefreq;	/* clock tick on the top level widget */

#include "xcalim_ad.h" //fallbackResources

void
main(argc, argv)
	int    argc;
	char   *argv[];
{
    Dimension       width;
	Widget          header;

	toplevel = XtAppInitialize(&appContext, "XCalim", Options,
            XtNumber(Options), &argc, argv, fallbackResources, NULL, 0);

	if (argc != 1) fprintf(stderr, "%s: Error in arguments\n", argv[0]);

	XtGetApplicationResources(toplevel, (void *) &appResources, Resources,
            XtNumber(Resources), (ArgList) NULL, 0);

	/* If reverse video invert default colour settings */
	if (appResources.reverseVideo) {
		Colour old = appResources.today;
		appResources.today.fg = old.bg;
		appResources.today.bg = old.fg;
	}

	if (appResources.otheruser) AlterTitles();
	
	InitMonthEntries();

	XtAppAddActions(appContext, appActions, XtNumber(appActions));
	XtAugmentTranslations(toplevel, XtParseTranslationTable(quitTranslations));

	SetUpdateFreq(appResources.format);

    /* Set today time */
	long ti;
	(void) time(&ti);
    ConvDate(localtime(&ti), &today);

	InitAlarms();

    parent = XtVaCreateManagedWidget("main", formWidgetClass, toplevel, 
            XtNdefaultDistance, 0,
            NULL);
    header = XtVaCreateManagedWidget("header", formWidgetClass, parent, 
            XtNtop,              XtChainTop,
            XtNdefaultDistance,  2,
            XtNbottom,           XtChainTop,
            XtNleft,             XtChainLeft,
            XtNresizable,        True,
            NULL);

    CreateHeaderButtons(header, &today);

    /* Ideally, if running as a daemon, we would not start the other two widgets
    either, but if we don't, the program complains and terminates, so I am
    choosing not to load only this one because it is the most heavy widget. */
    if (!appResources.daemon) width = NewMonthStrip(&today, NULL, header);

	XtSetMappedWhenManaged(toplevel, False);
    XtRealizeWidget(toplevel);

    delWin = XInternAtom(XtDisplay(toplevel), "WM_DELETE_WINDOW", False);
    (void) XSetWMProtocols(XtDisplay(toplevel), XtWindow(toplevel), &delWin, 1);

    if (!appResources.daemon) {
        XtVaSetValues(header, XtNwidth, width, NULL);
        XtMapWidget(toplevel);
        if (appResources.initialMemo) 
            XtCallCallbacks(XtNameToWidget(header, "memo"), XtNcallback, NULL);
        if (appResources.initialEdit) StartEditing(toplevel, &today, NULL);
        /* Without this the initial month is not mapped correctly if the window
        is resized. */
        XtCallCallbacks(XtNameToWidget(header, "current"), XtNcallback, NULL);
    }
	XtAppMainLoop(appContext);
	exit(0);
}

/*
 *	Adjust height of the command box
 */
static void
AdjustHeights(memo, date, help)
	Widget	memo;
	Widget	date;
	Widget	help;
{
	Dimension hm, hd, hh, max;

	hm = memo ? wHeight(memo): 0;
	hd = date ? wHeight(date): 0;
	hh = help ? wHeight(help): 0;
	
	max = hm;
	max = (hd > max) ? hd : max;
	max = (hh > max) ? hh : max;

	if (hm && hm < max)
		SetWidgetHeightMax(memo, hm, max);
	if (hd && hd < max)
		SetWidgetHeightMax(date, hd, max);
	if (hh && hh < max)
		SetWidgetHeightMax(help, hh, max);
}

void
SetWidgetHeightMax(w, h, max)
	Widget		w;
	Dimension	 h;
	Dimension 	max;
{
	int	adj;

	adj = ((int)max - (int)h)/2;
	if (adj > 0) {
		XtVaSetValues(w, 
                XtNheight, max,
                XtNfromVert, NULL,
                NULL);
	}
}

/*
 * If we are not dealing with our calendar entries add
 * (user-name)
 * to the end of the date format strings
 */
void
AlterTitles()
{
	char	us[16];
	char	fmt[256];

	(void) sprintf(us, " (%s)", appResources.otheruser);

	/* I am unsure whether it is safe to free these strings */
#define cstr(v) { strcpy(fmt, v); strcat(fmt, us); v = XtNewString(fmt); }
	cstr(appResources.format);
	cstr(appResources.stripfmt);
	cstr(appResources.editfmt);
#undef cstr
}

/*
 * Flip help state
 */
void
HelpShow(w, Pressed)
	Widget          w;
	Boolean         Pressed;
{
	Arg             arg[1];

	XtSetArg(arg[0], XtNbitmap, Pressed ? HelpPressPix : HelpPix);
	XtSetValues(w, arg, 1);
}


/*
 * Exit routine
 */
void
Leave(retval)
	int             retval;
{
	exit(retval);
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/*      This deals with the top level date `icon'                       */
/*                                                                      */
/*                                                                      */
/************************************************************************/
/*
 * Time management code
 * Set up a Date structure from today's information
 */
static void
ConvDate(tm, dp)
        struct tm      *tm;
        Date           *dp;
{
        dp->day = tm->tm_mday;
        dp->month = tm->tm_mon;
        dp->year = tm->tm_year + 1900;
        dp->wday = tm->tm_wday;
}

void
FmtTime(tm, buf, len, fmt)
        struct tm       *tm;
        char            *buf;
        int              len;
        char            *fmt;
{
	if (strftime(buf, len, fmt, tm) == 0) {
		strftime(buf, len, "%A %e %B %Y", tm);
	}
}

void
FmtDate(da, buf, len, fmt)
        Date    *da;
        char    *buf;
        int     len;
        char    *fmt;
{
	if (strfdate(buf, len, fmt, da) == 0) {
		strfdate(buf, len, "%A %e %B %Y", da);
	}
}
	
static void
DebugMkDate(w)
	Widget          w;
{
	static long     ti;
	struct tm      *tm;
	Date            yesterday;
	static int      firsttime;

	yesterday = today;

	if (ti == 0)
		(void) time(&ti);
	else 
		ti += updatefreq;
	tm = localtime(&ti);

    ConvDate(tm, &today);

	FmtTime(tm, date_area, sizeof(date_area), appResources.format);

	XtVaSetValues(w, XtNlabel, (XtArgVal) date_area, NULL);

	if (firsttime != 0) {
		if (yesterday.day != today.day) {
			ChangeHighlight(&yesterday, &today);
			AlarmFilePoll(tm);
		}
		UpdateMemo();
	}
	firsttime = 1;
	XtAppAddTimeOut(appContext, (updatefreq < 60) ? 25 : 500, DebugMkDate, (void *) w);
}

void
MkDate(w)
	Widget          w;
{
	long            ti;
	struct tm      *tm;
	Date            yesterday;
	static	int	firsttime;

	yesterday = today;

	(void) time(&ti);
	tm = localtime(&ti);

    ConvDate(tm, &today);

	FmtTime(tm, date_area, sizeof(date_area), appResources.format);

	XtVaSetValues(w, XtNlabel, (XtArgVal) date_area, NULL);

	if (firsttime != 0) {
		if (yesterday.day != today.day) {
			ChangeHighlight(&yesterday, &today);
			AlarmFilePoll(tm);
		}
		UpdateMemo();
	}
	firsttime = 1;
	
	ti = ClockSync(tm, updatefreq);
	XtAppAddTimeOut(appContext, ti * 1000, MkDate, (void *) w);
}

/*
 * Given a time structure and a frequency in secs
 * return no of secs to hit that interval
 */
long
ClockSync(tm, freq)
	struct	tm     *tm;
	long		freq;
{
	long	ti;

	ti = freq;
	if (ti > 1 && ti < 60)
		ti -= tm->tm_sec%freq;
	else
	if (ti >= 60 && ti < 3600)
		ti -= (tm->tm_min*60 + tm->tm_sec)%freq;
	else
	if (ti >= 3600)
		ti -= (tm->tm_hour*60*60 + tm->tm_min*60 + tm->tm_sec)%freq;
	return ti;
}

static int
CycleDays(settm)
	struct tm *settm;
{
	int	max = 0;
	int	maxday = 0;
	char	buf[BUFSIZ];
	struct tm tm;
	int	d;
	int	len;

	tm = *settm;
	for (d = 0; d < 7; d++) {
		tm.tm_wday = d;
		len = strftime(buf, sizeof buf, "%A", &tm);
		if (len > max) {
			maxday = d;
			max = len;
		}
	}
	return maxday;
}

static int
CycleMonths(settm)
	struct tm *settm;	
{
	int	max = 0;
	int	maxmon = 0;
	char	buf[BUFSIZ];
	struct tm tm;
	int	d;
	int	len;
	
	tm = *settm;
	for (d = 0; d < 11; d++) {
		tm.tm_mon = d;
		len = strftime(buf, sizeof buf, "%B", &tm);
		if (len > max) {
			maxmon = d;
			max = len;
		}
	}
	return maxmon;
}

/*
 * Look to see if we need to do something more than poll daily to change
 * the main toplevel strip
 */
static void
SetUpdateFreq(fmt)
	char	*fmt;
{	
	updatefreq = UpdateFreq(fmt);
	if (appResources.clocktick && updatefreq && 
	    updatefreq < appResources.clocktick)
		updatefreq = appResources.clocktick;
	if (updatefreq == 0)
		updatefreq = 24*60*60;
}


/*
 * Scan the time format string looking for an update frequency
 */
static int
UpdateFreq(fmt)
	char *fmt;
{
	int	update = 0;
#define	minu(v)	update = (update == 0 ? v : ((update < v) ? update : v))

	for(;*fmt; ++fmt) {
		if (*fmt == '%')
			switch(*++fmt) {
			case '\0':
				--fmt;
				break;
			case 'C':
			case 'c':
			case 'r':
			case 'S':
			case 's':
			case 'T':
			case 'X':
				minu(1);	/* every second */
				break;
			case 'M':
			case 'R':
				minu(60);	/* every minute */
				break;
			case 'H':
			case 'h':
			case 'I':
			case 'k':
			case 'l':
				minu(3600);	/* every hour */
				break;
			case 'p':
			case 'P':
				minu(43200);	/* AM/PM */
				break;
			}
	}
	return update;
}

/* Handle accelerator or close events pressed in windows */
/* TODO: This has been shamefully abused and needs to be put in a form compliant
with any good practices standard. */
static void
PressAButton(w, event, params, numb)
	Widget          w;
	XEvent         *event;
	String         *params;
	Cardinal       *numb;   //Must be =1
{
    Widget button;
    int i = atoi(*params);
    if (i < 2 || i == 11) w = XtParent(w);

    char *buttonNames[] = {
        "*save", 
        "*delete",
        "*quit",
        "*no",
        "*yes",
        "header.back",
        "header.next",
        "header.memo",
        "header.current",
        "*cancel",
        "*help",
        "*help",
    };

    // Paranoia, this should be really always true
    if ((button = XtNameToWidget(w, buttonNames[i])))
        XtCallCallbacks(button, XtNcallback, (XtPointer *) NULL);
}
