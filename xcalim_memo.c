/***
* module name:
	xcalim_memo.c
* function:
	Deal with popup memo file
	A single popup file is stored in a file called
	memo on the Calendar directory
***/
#include <stdio.h>
#include <ctype.h>
#include <X11/Xos.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Dialog.h>
#include "xcalim.h"

static XtCallbackRec callbacks[] = {
	{NULL, NULL},
	{NULL, NULL}
};
#define ClearCallbacks() memset((void *)callbacks, '\0', sizeof (callbacks))
#define argLD(N,V) { XtSetArg(args[nargs], N, V); nargs++; }

static String leaveTransl =
"<Key>n    :PressAButton(3)\n\
<Key>y     :PressAButton(4)\n\
<Key>Return:PressAButton(4)\n\
<Key>c     :PressAButton(9)\n\
<Key>Escape:PressAButton(3)";

/*
 * Structure for storing relavant data about the memo Edit
 */
typedef struct memoEdit {
	Widget          m_button;	/* widget of the control button */
	Widget          m_popup;	/* widget of editor popup */
	Widget          m_quit;		/* widget of quit button */
	Widget		m_edit;		/* widget of edit button */
	Widget		m_help;		/* widget of help button */
	Widget          m_save;		/* widget of save button */
	Boolean         m_savesens;	/* state of the save button */
	Widget          m_display;	/* widget of display title area */
	Widget          m_text;		/* the text area */
	Widget          m_today;	/* today's data */
	Widget          m_weekly;	/* widget of text image of weekly */
					/* events */
	String		m_weeklytext;	/* weekly text */
	Cardinal        m_size;		/* size of the buffer */
	char           *m_data;		/* pointer to malloc'ed data buffer */
} MemoEdit;

static MemoEdit memo;

static String   memoContents;

extern void     MemoHelp();		/* look in xcalim_help.c */

/*
 * Internal routines
 */
void            MemoPopup();
void            CloseMemo();
static void     CleanMemo();
static void     MemoCheckExit();
static void     MCheckDia();
static Boolean  WriteMemoFile();
static void	EditToday();
static int      NewlineCount();
static String   GetMemoFile();
static void     SaveMemoEdits();
static void     MemoTextChanged();
static void     FinishMemoEditing();
static void     YesCheck();
static void     NoCheck();
static void     CancelCheck();
static void	AdjustTitleHeight();

/*
 * Callback routine to display the memo file
 */

void
DoMemo(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	static Arg      args[1];
    Pixel           bg, fg;

	/* Make the button become a finish button */
	memo.m_button = w;
	callbacks[0].callback = FinishMemoEditing;
	callbacks[0].closure = NULL;
    XtVaGetValues(w, XtNbackground, &bg, XtNforeground, &fg, NULL);
	XtVaSetValues(w, XtNcallback, callbacks, XtNbackground, fg, XtNforeground, 
            bg, NULL);

	/*
	 * Get existing memo contents
	 * if the user is polling then re-read the file
	 */
	if (appResources.update && memoContents != NULL) {
		XtFree(memoContents);
		memoContents = NULL;
	}
	if (memoContents == NULL)
		memoContents = GetMemoFile();

	/*
	 * Set up the popup widget for editing
	 */
	MemoPopup();
}


/*
 * Get old contents from a memo file if any
 */
static String
GetMemoFile()
{

	if (FoundCalendarDir && access(appResources.memoFile, F_OK) == 0)
		return ReadCalendarFile(NULL, appResources.memoFile);
	return NULL;
}


/*
 * Do the biz to popup an edit style window
 */
void
MemoPopup()
{
	Widget          et, lw;
	Widget          frame;
	Arg             args[10];
	Cardinal        nargs;
	String          str;
	MonthEntry     *me;
	Dimension       charHeight;

	/*
	 * set up edit buffer
	 */
	if (memoContents)
		memo.m_size = appResources.textbufsz + strlen(memoContents) + 1;
	else
		memo.m_size = appResources.textbufsz;
	memo.m_data = XtMalloc(memo.m_size);

	if (memoContents)
		strcpy(memo.m_data, memoContents);
	else
		*memo.m_data = '\0';
	memo.m_popup = XtCreatePopupShell("memo", topLevelShellWidgetClass, toplevel, NULL, 0);

	/*
	 * The first title line
	 */
	et = XtCreateManagedWidget("memoPanel", panedWidgetClass, memo.m_popup, NULL, 0);

	nargs = 0;
	argLD(XtNshowGrip, False);
	argLD(XtNskipAdjust, True);
	argLD(XtNdefaultDistance, 1);
	frame = XtCreateManagedWidget("title", formWidgetClass, et, args, nargs);
	/*
	 * containing some buttons for controlling the world
	 */
	/*
	 * Take label "quit" from resources
	 */
	callbacks[0].callback = FinishMemoEditing;
	callbacks[0].closure = NULL;
	nargs = 0;
	argLD(XtNcallback, callbacks);
	argLD(XtNfromHoriz, NULL);
	argLD(XtNleft, XtChainLeft);
	argLD(XtNright, XtChainLeft);
	lw = memo.m_quit = XtCreateManagedWidget("quit", commandWidgetClass, frame, args, nargs);
    XtUnmanageChild(lw);

	/*
	 * Edit todays file from here as well
	 * Take label from resources
	 */
	if (MyCalendar) {
		callbacks[0].callback = EditToday;
		callbacks[0].closure = (void *) 0;
		nargs = 0;
		argLD(XtNcallback, callbacks);
		argLD(XtNfromHoriz, NULL);
		argLD(XtNleft, XtChainLeft);
		argLD( XtNright, XtChainLeft);
		memo.m_edit = lw = XtCreateManagedWidget("edit", commandWidgetClass, frame, args, nargs);
	}
	/*
	 * If we are dealing with  help then do it now
	 */
	if (appResources.giveHelp) {
		/* Take label "help" from resources */
		callbacks[0].callback = MemoHelp;
		callbacks[0].closure = (void *) 0;
		nargs = 0;
		argLD(XtNcallback, callbacks);
		argLD(XtNfromHoriz, lw);
		argLD(XtNleft, XtChainLeft);
		argLD( XtNright, XtChainLeft);
		memo.m_help = lw = XtCreateManagedWidget("help", commandWidgetClass, frame, args, nargs);
	}

	/*
	 * The remaining bit here is a date label
	 */
	nargs = 0;
	argLD(XtNlabel, date_area);
	argLD(XtNborderWidth, 0);
	argLD(XtNfromHoriz, lw);
	argLD(XtNfromVert, NULL);
	argLD(XtNvertDistance, 2);
	argLD(XtNleft, XtChainLeft);
	argLD(XtNright, XtChainRight);
	lw = memo.m_display = XtCreateManagedWidget("date", labelWidgetClass, frame, args, nargs);

	/*
	 * Details for today
	 */
	me = GetMonthEntry(today.year, today.month);
	nargs = 0;
	str = me->me_have[today.day];
	if (str == NULL)
		str = "";
	argLD(XtNstring, str);
	argLD(XtNdisplayCaret, False);
	argLD(XtNeditType, XawtextRead);
	memo.m_today = XtCreateManagedWidget("display", asciiTextWidgetClass, et, args, nargs);
	{
		Dimension       height;

		XtSetArg(args[0], XtNheight, &height);
		XtGetValues(memo.m_today, args, 1);
		charHeight = height;
		height = height * NewlineCount(str);
		XtSetArg(args[0], XtNheight, height);
		XtSetValues(memo.m_today, args, 1);
	}

	AdjustTitleHeight(memo.m_quit,
			  MyCalendar ? memo.m_edit : NULL,
			  appResources.giveHelp ? memo.m_help : NULL,
			  memo.m_display);

	/*
	 * Weekly details - the data for today + an edit button
	 * The header to this is a form
	 */
	nargs = 0;
	argLD(XtNshowGrip, False);
	argLD(XtNskipAdjust, True);
	argLD(XtNdefaultDistance, 1);
	frame = XtCreateManagedWidget("weeklyMemo", formWidgetClass, et, args, nargs);
	/*
	 * Take label "edit" from resources
	 */
	if (MyCalendar) {
		callbacks[0].callback = DoWeekly;
		callbacks[0].closure = (void *) & memo;
		nargs = 0;
		argLD(XtNcallback, callbacks);
		argLD(XtNfromHoriz, NULL);
		argLD(XtNleft, XtChainLeft);
		argLD(XtNright, XtChainLeft);
		lw = XtCreateManagedWidget("weeklyEdit", commandWidgetClass, frame, args, nargs);
	}
		
	/*
	 * Say this is a weekly commitment
	 */
	nargs = 0;
	argLD(XtNshowGrip, True);
	argLD(XtNborderWidth, 0);
	argLD(XtNfromHoriz, MyCalendar ? lw : NULL);
	argLD(XtNfromVert, NULL);
	argLD(XtNvertDistance, 2);
	argLD(XtNleft, XtChainLeft);
	argLD(XtNright, XtChainRight);
	lw = XtCreateManagedWidget("weeklyTitle", labelWidgetClass, frame, args, nargs);

	/*
	 * Details for today
	 */
	nargs = 0;
	if (memo.m_weeklytext)
		XtFree(memo.m_weeklytext);
	memo.m_weeklytext = str = GetWeeklyFile(today.wday);
	if (str == NULL)
		str = "";
	argLD(XtNstring, str);
	argLD(XtNdisplayCaret, False);
	argLD(XtNeditType, XawtextRead);
	if (charHeight)
		argLD(XtNheight, NewlineCount(str) * charHeight);
	memo.m_weekly = XtCreateManagedWidget("display", asciiTextWidgetClass, et, args, nargs);

	/*
	 * Another form with some buttons
	 */
	nargs = 0;
	argLD(XtNshowGrip, False);
	argLD(XtNskipAdjust, True);
	argLD(XtNdefaultDistance, 1);
	frame = XtCreateManagedWidget("memoMiddle", formWidgetClass, et, args, nargs);
	if (MyCalendar) {
		/*
		 * Take label "save" from resources
		 */
		callbacks[0].callback = SaveMemoEdits;
		callbacks[0].closure = (void *) & memo;
		nargs = 0;
		argLD(XtNcallback, callbacks);
		argLD(XtNfromHoriz, NULL);
		argLD(XtNleft, XtChainLeft);
		argLD(XtNright, XtChainLeft);
		argLD(XtNsensitive, False);
		lw = memo.m_save = XtCreateManagedWidget("save", commandWidgetClass, frame, args, nargs);
		memo.m_savesens = False;
	}
	/*
	 * Say this is a memo edit
	 */
	nargs = 0;
	argLD(XtNshowGrip, True);
	argLD(XtNborderWidth, 0);
	argLD(XtNfromHoriz, MyCalendar ? lw : NULL);
	argLD(XtNfromVert, NULL);
	argLD(XtNvertDistance, 2);
	argLD(XtNleft, XtChainLeft);
	argLD(XtNright, XtChainRight);
	lw = XtCreateManagedWidget("memoTitle", labelWidgetClass, frame, args, nargs);

	/*
	 * The text widget is in the pane below
	 * The Scroll Attributes are controlled from the application
	 * defaults file
	 */
	callbacks[0].callback = MemoTextChanged;
	callbacks[0].closure = (void *) & memo;
	nargs = 0;
	argLD(XtNstring, memo.m_data);
	argLD(XtNeditType, XawtextEdit);
	argLD(XtNlength, memo.m_size);
	argLD(XtNuseStringInPlace, True);
	argLD(XtNcallback, callbacks);
	memo.m_text = XtCreateManagedWidget("memoText", asciiTextWidgetClass, et, args, nargs);

	XtOverrideTranslations(memo.m_popup,
            XtParseTranslationTable("<Message>WM_PROTOCOLS: PopDownMemo()"));
    XawTextSetInsertionPoint(memo.m_text, XawTextLastPosition(memo.m_text));
	XtPopup(memo.m_popup, XtGrabNone);
    XtSetKeyboardFocus(memo.m_popup, memo.m_text);
    (void)XSetWMProtocols(XtDisplay(memo.m_popup), XtWindow(memo.m_popup),
            &delWin, 1);
}

/*
 * Adjust title line height
 * possibly 4 objects
 */
static void
AdjustTitleHeight(quit, edit, help, label)
	Widget		quit;
	Widget		edit;
	Widget		help;
	Widget		label;
{
	int		hq, he, hh, hl;
	int		max;

	hq = wHeight(quit);
	he = edit ? wHeight(edit) : 0;
	hh = help ? wHeight(help): 0;
	hl = wHeight(label);

	max = hq;
	max = (he > max) ? he : max;
	max = (hh > max) ? hh : max;
	max = (hl > max) ? hl : max;

	if (hq < max)
		SetWidgetHeightMax(quit, hq, max);
	if (he && he < max)
		SetWidgetHeightMax(edit, he, max);
	if (hh & hh < max)
		SetWidgetHeightMax(help, hh, max);
	if (hl < max)
		SetWidgetHeightMax(label, hl, max);
}

/*
 * This callback starts editing today
 */
static void
EditToday(w, closure, call_data)
	Widget		w;
	void *         closure;
	void *         call_data;
{	

	StartEditing(w, &today, w);
}

/*
 * Count newlines in a string
 */
static int
NewlineCount(str)
	String          str;
{
	register int    sum = 0;

	while (*str)
		if (*str++ == '\n')
			sum++;
	/* Add one line - assume last line does NOT have an nl */
	sum++;
	/* ignore a final newline */
	if (str[-1] == '\n')
		sum--;
	if (sum <= 0)
		sum = 1;
	return (sum > appResources.maxDisplayLines ? appResources.maxDisplayLines : sum);
}

/*
 * Entry point from outside when today's text changed
 */
void
UpdateMemo()
{
	Arg             args[1];
	String          str;
	MonthEntry     *me;

	/*
	 * if the button widget is zero then we are displaying nothing
	 */
	if (memo.m_button == 0)
		return;

	me = GetMonthEntry(today.year, today.month);
	str = me->me_have[today.day];
	if (str == NULL)
		str = "";
	XtSetArg(args[0], XtNstring, str);
	XtSetValues(memo.m_today, args, 1);

	XtSetArg(args[0], XtNlabel, date_area);
	XtSetValues(memo.m_display, args, 1);

	if (memo.m_weeklytext)
		XtFree(memo.m_weeklytext);
	memo.m_weeklytext = str = GetWeeklyFile(today.wday);
	if (str == NULL)
		str = "";
	XtSetArg(args[0], XtNstring, str);
	XtSetValues(memo.m_weekly, args, 1);

}

/*
 * Poll call from the alarm timeout
 */
void
MemoPoll()
{
	int             size;
	Arg             args[10];
	int             nargs;

	if (memo.m_button == 0)
		return;
	if (memo.m_savesens == True)
		return;

	if (memoContents)
		XtFree(memoContents);
	memoContents = GetMemoFile();
	if (memoContents) {
		if (strcmp(memoContents, memo.m_data) == 0)
			return;
		size = strlen(memoContents) + 1;
		if (size > memo.m_size) {
			size += appResources.textbufsz;
			XtFree(memo.m_data);
			memo.m_data = XtMalloc(memo.m_size = size);
		}
		strcpy(memo.m_data, memoContents);
	} else
		*memo.m_data = '\0';

	nargs = 0;
	argLD(XtNstring, memo.m_data);
	argLD(XtNlength, memo.m_size);
	argLD(XtNuseStringInPlace, True);
	XtSetValues(memo.m_text, args, nargs);
}

/*
 * Call backs for various buttons
 */
/* ARGSUSED */
static void
MemoTextChanged(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	register MemoEdit *memo = (MemoEdit *) closure;

	if (MyCalendar) {
		memo->m_savesens = True;
		XtSetSensitive(memo->m_save, True);
	}
}

/*
 * Callback routines
 */
/* ARGSUSED */
static void
SaveMemoEdits(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	MemoEdit       *memo = (MemoEdit *) closure;

	if (WriteMemoFile(memo) == False)
		return;
	if (memoContents) {
		XtFree(memoContents);
		memoContents = XtNewString(memo->m_data);
	}
	memo->m_savesens = False;
	XtSetSensitive(memo->m_save, False);
}

/*
 * Write the memo file out
 */
static          Boolean
WriteMemoFile(memo)
	MemoEdit       *memo;
{
	Cardinal        len = strlen(memo->m_data);
	String          fname;
	int             fd;

	if (len == 0) {
		unlink(appResources.memoFile);
		return (True);
	}
	/*
	 * First let's see if we have to create the toplevel directory
	 */
	if (!NeedTop())
		return (False);

	fname = appResources.memoFile;

	if ((fd = open(fname, O_WRONLY | O_TRUNC | O_CREAT, 0666)) < 0) {
		XBell(XtDisplay(toplevel), 0);
		fprintf(stderr, "xcalim: Could not open %s/%s for writing.\n", MapStem, fname);
		perror("xcalim: open");
		fflush(stderr);
		return (False);
	}
	if (write(fd, memo->m_data, len) != len) {
		XBell(XtDisplay(toplevel), 0);
		fprintf(stderr, "xcalim: Write error %s/%s file.\n", MapStem, fname);
		perror("xcalim: write");
		fflush(stderr);
		close(fd);
		return (False);
	}
	close(fd);
	return (True);
}

void
CloseMemo(w, event, params, numb)
	Widget          w;
	XEvent         *event;
	String         *params;
	Cardinal       *numb;
{
    FinishMemoEditing(w, (void *) NULL, (void *) NULL);
}

static void
FinishMemoEditing(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{

	if (memo.m_savesens == True)
		MemoCheckExit();
	else
		CleanMemo();
}

static void
CleanMemo()
{
	static MemoEdit	zerom;
    Pixel           bg, fg;

	callbacks[0].callback = DoMemo;
	callbacks[0].closure = NULL;
    XtVaGetValues(memo.m_button, XtNforeground, &fg, XtNbackground, &bg, NULL);
	XtVaSetValues(memo.m_button, XtNcallback, callbacks, XtNforeground, bg,
            XtNbackground, fg, NULL);
	XtSetSensitive(memo.m_button, True);

	XtPopdown(memo.m_popup);
	XtDestroyWidget(memo.m_popup);
	XtFree(memo.m_data);
	XtFree(memo.m_weeklytext);
	memo = zerom;
}

static void
MemoCheckExit()
{
	DialogPopup(memo.m_quit, MCheckDia, &memo, NULL);
}

static void
MCheckDia(pop, ed)
	Widget          pop;
	MemoEdit       *ed;
{
	Widget          dia;

	XtSetSensitive(memo.m_button, False);
	XtSetSensitive(memo.m_save, False);

	/* Take "Save file?" from resources */
	dia = XtCreateManagedWidget("memocheck", dialogWidgetClass, pop, NULL, 0);
	XawDialogAddButton(dia, "yes", YesCheck, ed);
	XawDialogAddButton(dia, "no", NoCheck, ed);
	XawDialogAddButton(dia, "cancel", CancelCheck, ed);
    XtOverrideTranslations(XtParent(dia),
            XtParseTranslationTable("<Message>WM_PROTOCOLS: PressAButton(4)"));
    XtOverrideTranslations(dia, XtParseTranslationTable(leaveTransl));
}

/* ARGSUSED */
static void
YesCheck(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	SaveMemoEdits(w, closure, call_data);
	CleanMemo();

	XtDestroyWidget(XtParent(XtParent(w)));
}

/* ARGSUSED */
static void
NoCheck(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	CleanMemo();
        XtDestroyWidget(XtParent(XtParent(w)));
}

/* ARGSUSED */
static void
CancelCheck(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
    MemoEdit *ed = (MemoEdit *) closure;
    XtSetSensitive(ed->m_save, True);
	XtSetSensitive(memo.m_button, True);
    XtDestroyWidget(XtParent(XtParent(w)));
}
