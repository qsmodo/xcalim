/* #define	ALLOW_OTHER_WRITE */
/* Normally, a user can look at someone else's Calendar files with the   */
/* -u flag. For safety, they are prohibited by xcalim from writing. Define */
/* ALLOW_OTHER_WRITE to allow anyone to write to anyone else's files     */
/* except, of course, they will be protected by normal file permissions  */

/*
 *	On 32 bit machines we can pack the date into one word
 *	if not we have to use two callbacks
 *	so undef if not true
 *	and fiddle with the definitions below
 */
/* #define	LONG_IS_32_BITS */

/* pack year and month into Cardinals */
#ifdef LONG_IS_32_BITS
#define	DatePack(w, d, m, y)	((((w)&0x7)<<21) | (((d)&0x1f)<<16) | (((m)&0xf)<<12) | ((y)&0xfff))
#define YrUnpack(v)	((v)&0xfff)
#define MoUnpack(v)	(((v)>>12)&0xf)
#define DyUnpack(v)	(((v)>>16)&0x1f)
#define WdUnpack(v)	(((v)>>21)&0x7)
#else /*  LONG_IS_32_BITS */
#define	DatePack(m, y)	((((m)&0xf)<<12) | ((y)&0xfff))
#define DayPack(w, d)	((((w)&0x7)<<5) | ((d)&0x1f))
#define YrUnpack(v)	((v)&0xfff)
#define MoUnpack(v)	(((v)>>12)&0xf)
#define DyUnpack(v)	((v)&0x1f)
#define WdUnpack(v)	(((v)>>5)&0xf)
#endif /* LONG_IS_32_BITS */

/*
 *	Foreground/Background colours
 */
typedef struct
{	Pixel	bg;
	Pixel	fg;
} Colour;

/*
 *	Time structure
 */
typedef struct tm Tm;

/*
 *	resources used by xcalim.c
 */
struct resources
{
	Boolean	alarmScan;	/* Debug switch for alarm system */
	Boolean	reverseVideo;	/* Display in Reverse video */
	Boolean	daemon;	/* Daemonize */
	Boolean	justQuit;	/* Just quit, don't ask */
	Boolean	markToday;	/* Mark today with today's colours */
	Boolean	calCompat;	/* True if handle files like xcalendar */
	Boolean giveHelp;	/* True if help is needed (default) */
	Boolean helpFromFile;	/* True if read help text from a file */
	String	helpfile;	/* Where to find the help file */
	Boolean initialEdit;	/* Pop up today's Edit on startup if True */
	Boolean	initialMemo;	/* Pop up memo box on start */
	String	format;		/* Date format string to use in the main box */
	String	editfmt;	/* Date format in edit windows */
	String	mon[12];	/* Long month names */
	String	smon[12];	/* Short month names */
	String	day[7];		/* Day names - full */
	String	sday[7];	/* Short day names */
	String	weekly;		/* Title of weekly edit strip */
	Colour	today;		/* What to mark today with */
	String	directory;	/* Directory under home where Calendar files */
				/* can be found */
	int	textbufsz;	/* Text buffer size for editing */
	Dimension minstripwidth; /* Minimum strip width */
	Boolean	alarms;		/* false - no alarms, true - alarms */
	Boolean	execalarms;	/* false - no exec alarms, true - exec alarms */
	XtIntervalId interval_id;/* store XtAddTimeOut value */
	int	clocktick;	/* how often the toplevel widget is polled */
	int	update;		/* interval between peeks (60 secs) */
	int	autoquit;	/* Automatically delete message boxes */
	String	countdown;	/* Comma separated countdown string for alarms */
	String	cmd;		/* command to execute for every alarm */
	String	alarmleft;	/* string containing a message - %d mins left */
	String	alarmnow;	/* string containing a message - Now! */
	String	private;	/* string containing the word - Private */
	String	already;	/* date format for "Already editing day month year" */
	String	alreadyWeekly;	/* date format for "Already editing day" */
	Boolean	useMemo;	/* true use top-level memo button, false - don't */
	Boolean memoLeft;	/* True if on left of date, false otherwise */
	String	memoFile;	/* name of the file where the memo data is stored */
	int	maxDisplayLines;/* maximum number of lines that we want to */
				/* allow the top half of a memo window to */
				/* stretch to */
	char	*otheruser;	/* the name of another user */
	Dimension squareW; /* Width of each box/square in the calendar */
	Dimension squareH; /* Height of each box/square minus the day label height */
	Dimension labelH; /* Day label height in eeach box/square */
};

#ifndef ALLOW_OTHER_WRITE
#define	MyCalendar	(appResources.otheruser == NULL)
#else
#define MyCalendar	(1)
#endif

extern	struct resources	appResources;
extern  Atom   delWin;

/*
 *	Application context for the program - set in xcalim.c
 */
extern XtAppContext	appContext;
/*
 *      Date structure
 */
typedef struct
{       Cardinal        day;
        Cardinal        month;
        Cardinal        year;
        Cardinal        wday;
} Date;
 
/*
 *	A month entry
 */
typedef struct me
{
	Cardinal	me_year;	/* map year */
	Cardinal	me_month;	/* which month */
	String		me_have[32];	/* if a file present for the day */
					/* then will have a non-zero entry */
	int		me_type;	/* type of displayed strip */
} MonthEntry;

#define ME_MONTHLY	1		/* `Normal' monthly strip */
#define ME_WEEKLY	2		/* Weekly strip */

/*
 *	An instance of the strip
 */
typedef	struct	instance
{	struct	instance *i_next;	/* next object */
	Widget	i_w;			/* the widget top level */
	Widget	i_day_label[32];	/* the handle to the label on each day */
					/* so we can change dates at night */
	Widget	i_day_info[32];		/* The info field for this date - so */
					/* we can sensitise/desensitise on editing */
	Colour	i_col;			/* what the fg/bg colours used to be */
	XFontStruct	*i_font;	/* what the font was */
} Instance;

Instance *RegisterMonth();
Instance *FindInstanceList();

/*
 *	Alarm structure
 *	one of these exists for each event in the timeout queue
 *	the list is sorted in event order
 */
typedef struct _alarm
{	struct _alarm *next;	/* pointer to next alarm */
	String	alarm;		/* alarm string */
	String	what;		/* what is message */
	int	alarm_mm;	/* hour*60 + min */
	int	alarm_state;	/* what time left to `real' timeout */
	Boolean isAction;	/* more than alarm */
} Alarm;

/*
 *	We occasionally need these
 */
extern	Widget	toplevel, parent;
extern	Date	today;
extern  char	date_area[];
extern	Boolean	FoundCalendarDir;
extern	char    *MapStem;		/* pointer to the string which is */
					/* where the map data is stored */

extern	Boolean	FoundCalendarDir;	/* whether the Calendar directory */
					/* exists */

/*
 *	Global routines
 */
void	MemoPopup();
void	InitAlarms();
void	FmtTime();
void	Leave();
void	Fatal();
Dimension wHeight();
void    MkDate();
void    CreateHeaderButtons();
void	SetWidgetHeightMax();
void	SetDate();
void    PopdownHelp();
void	AskLeave();
Widget  DialogPopup();
void	TextCal();
void	LoadFromPrimary();
void	WeekDay();
void	CloseMemo();
void	PopUpMemo();
void	Nr();
void	PopDownShell();
void	ScrollCal();
void	StripHelp();
void	DoMemo();
void	DoWeekly();
void	MemoPoll();
Dimension	NewMonthStrip();
void	InitMonthEntries();
void	ChangeHighlight();
void	NoEditIsPossible();
void	NoDayEditIsPossible();
void	StartEditing();
//void	PressAButton();
MonthEntry *GetMonthEntry();
MonthEntry *GetWeeklyEntry();
void	AlarmFilePoll();
String	ReadCalendarFile();
void	UpdateMemo();
String	MakeWeeklyName();
String	GetWeeklyFile();
Boolean	NeedTop();
void	AppendText();
void	ButtonOff();
void	ButtonOn();
void	FmtDate();
long	ClockSync();
Cardinal	NumberOfDays();
Cardinal	FirstDay();

time_t	time();
