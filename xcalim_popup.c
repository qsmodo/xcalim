/***
* module name:
	xcalim_popup.c
* function:
	Deal with various popups for xcalim
	There are two main ones:
	a)	the centre button causes a popup date selection popup
	b)	the right button causes an exit popup
***/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include "xcalim.h"

static void     AskDialog();
static void     NoCal();
static void     YesCal();
static char    *DateParse();
static int      MonScan();
static void     LeaveDialog();
static void     NoLeave();
static void     YesLeave();
static void     NoEdit();

#define argLD(N,V) { XtSetArg(args[nargs], N, V); nargs++; }

static String leaveTransl =
"<Key>n    :PressAButton(3)\n\
<Key>y     :PressAButton(4)\n\
<Key>Return:PressAButton(4)\n\
<Key>Escape:PressAButton(3)";

/*
 * This routine deals with most of the work to create a dialog popup, it is
 * passed a function which is called to create the dialog box
 * 
 * The widget here is used for positioning, these popups are always children of
 * the toplevel widget
 *
 * These widgets are generally called from a Command widget button.
 * if this is the case, we make that button non-sensitive and
 * use the pop-down callback to turn them on again (see xcalim_buts.c)
 */
Widget
DialogPopup(w, fn, arg, but)
	Widget          w;
	void            (*fn) ();
	void *         arg;
	Widget		but;
{
	Widget          pop;
	Arg             args[10];
	int		nargs;
	Position        x, y;
	Position        nx, ny;
	Dimension       width, height, border;

	/* Get the position of the toplevel so we 
	 * can position the dialog box properly */
	XtSetArg(args[0], XtNwidth, &width);
	XtSetArg(args[1], XtNheight, &height);
	XtGetValues(w, args, 2);
	XtTranslateCoords(w, (Position) (width / 2),
			  (Position) (height / 2), &x, &y);

	/* * Create a popup to hold the dialog */
	nargs = 0;
	argLD(XtNallowShellResize, True);
	argLD(XtNinput, True);
	argLD(XtNx, x);
	argLD(XtNy, y);
	argLD(XtNsaveUnder, False);

	pop = XtCreatePopupShell("question", transientShellWidgetClass, toplevel, 
            args, nargs);

	if (but && XtIsSubclass(but, commandWidgetClass)) ButtonOff(but, pop);

	/* * Set up the dialog */
	(*fn) (pop, arg);

	XtRealizeWidget(pop);

	/* * We can now worry if this box is actually off the screen */
	XtSetArg(args[0], XtNwidth, &width);
	XtSetArg(args[1], XtNheight, &height);
	XtSetArg(args[2], XtNborderWidth, &border);
	XtGetValues(pop, args, 3);

	border <<= 1;
	XtTranslateCoords(pop, (Position) 0, (Position) 0, &nx, &ny);

	if ((int)(nx + width + border) > WidthOfScreen(XtScreen(toplevel)))
		nx = WidthOfScreen(XtScreen(toplevel)) - width - border;
	else
		nx = x;

	if ((int)(ny + height + border) > HeightOfScreen(XtScreen(toplevel)))
		ny = HeightOfScreen(XtScreen(toplevel)) - height - border;
	else
		ny = y;

	if (nx != x || ny != y) {
		XtSetArg(args[0], XtNx, nx);
		XtSetArg(args[1], XtNy, ny);
		XtSetValues(pop, args, 2);
	}
	XtPopup(pop, XtGrabNone);
    (void) XSetWMProtocols(XtDisplay(pop), XtWindow(pop), &delWin, 1);
    return(pop);
}

/************************************************************************/
/*									*/
/*									*/
/*	Deals with middle button presses - ask for a date		*/
/*									*/
/*									*/
/************************************************************************/

/*
 * SetDate - ask for a date and start a calendar
 * This is an action routine
 */
/* ARGSUSED */
void
SetDate(w, event, params, numb)
	Widget          w;
	XEvent         *event;
	String         *params;
	Cardinal       *numb;
{
	DialogPopup(toplevel, AskDialog, NULL, w);
}

/* ARGSUSED */
static void
AskDialog(pop, noop)
	Widget          pop;
	Cardinal        noop;
{
	Widget          dia;
	Arg             args[2];
	WidgetList      children;	/* which is Widget children[] */
	Cardinal        num_children;
	int             i;

	/* Take from args: "Enter mm yyyy?" */
	XtSetArg(args[0], XtNvalue, "");
	dia = XtCreateManagedWidget("newdate", dialogWidgetClass, pop, args, 1);
	XawDialogAddButton(dia, "ok", YesCal, dia);
	XawDialogAddButton(dia, "cancel", NoCal, pop);
	/*
	 * I would like to add CR translations to the text box the only way
	 * to get the widget seems to be to use an R4 feature to get the
	 * WidgetList
	 */
	XtSetArg(args[0], XtNchildren, &children);
	XtSetArg(args[1], XtNnumChildren, &num_children);
	XtGetValues(dia, (ArgList) args, 2);
	for (i = 0; i < num_children; i++) {
		if (XtClass(children[i]) == asciiTextWidgetClass) {
			/* Bingo */
			XtOverrideTranslations(
					       children[i],
					       XtParseTranslationTable("<Key>Return: SetDateAction()")
				);

		} else
		if (XtClass(children[i]) == labelWidgetClass) {
			XtSetArg(args[0], XtNresizable, True);
			XtSetValues(children[i], args, 1);
		}
	}

}

/*
 * No we don't want a specified date
 * Closure here is the pop shell
 */
/* ARGSUSED */
static void
NoCal(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	XtDestroyWidget((Widget) closure);
}

/*
 * Yes we do want a specified date
 * Closure here is the dialog widget
 */
/* ARGSUSED */
static void
YesCal(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	Widget          dia;
	Arg             args[2];
	Date            wanted;
	char           *errstr;

	dia = (Widget) closure;
	/*
	 * Parse the string
	 */
	if (errstr = DateParse(XawDialogGetValueString(dia), &wanted)) {
		/*
		 * insert an error							 * message in the widget
		 */
		XtSetArg(args[0], XtNlabel, errstr);
		XtSetValues(dia, args, 1);
		XBell(XtDisplay(toplevel), 0);
		return;
	}
	XtDestroyWidget(XtParent(dia));
	NewMonthStrip(&wanted, NULL);
}

/*
 * Action mapped to by CR in the dialog
 */
/* ARGSUSED */
void
TextCal(w, event, params, numb)
	Widget          w;
	XEvent         *event;
	String         *params;
	Cardinal       *numb;
{
	YesCal(w, (void *) XtParent(w), 0);	/* parent of text widget is */
						/* the dialog box */
}

/*
 * Parse a date string
 */
static char    *
DateParse(str, da)
	register char  *str;
	Date           *da;
{
	register char  *wk;
	int             lastc;
	int             mo;

	*da = today;

	wk = str;
	while (isspace(*wk))
		wk++;
	if (*wk == '\0')
		return ("No data found");
	str = wk;
	if (isdigit(*str)) {
		while (isdigit(*str))
			str++;
		lastc = *str;
		*str++ = '\0';
		mo = atoi(wk);
		if (mo < 1 || mo > 12)
			return ("Illegal month number");
		da->month = mo - 1;
	} else if (isalpha(*str)) {	/* be kind - allow month names */
		while (isalpha(*str)) {
			if (isupper(*str))
				*str = tolower(*str);
			str++;
		}
		lastc = *str;
		*str++ = '\0';
		mo = MonScan(wk);
		if (mo < 0)
			return ("Cannot find month name");
		if (mo < da->month)
			da->year++;
		da->month = mo;
	}
	if (lastc) {
		wk = str;
		while (isspace(*wk))
			wk++;
		str = wk;
		if (*str)
			da->year = atoi(wk);
	}
	return (NULL);
}

/*
 * Given a string look in our database for a number
 */
static int
MonScan(monstr)
	char           *monstr;
{
	char           *a, *b;
	int             ca, cb;
	int             mon;

	for (mon = 0; mon < 12; mon++)
		for (a = monstr, b = appResources.mon[mon];;) {
			ca = *a++;
			if (ca == '\0')
				return (mon);
			if (isupper(ca))
				ca = tolower(ca);
			cb = *b++;
			if (cb == '\0')
				break;
			if (isupper(cb))
				cb = tolower(cb);
			if (ca != cb)
				break;
		}
	return (-1);
}

/************************************************************************/
/*									*/
/*									*/
/*		Deals with right button presses - exit			*/
/*									*/
/*									*/
/************************************************************************/
/*
 * Get out - possibly
 */
/* ARGSUSED */
void
AskLeave(w, event, params, numb)
	Widget          w;
	XEvent         *event;
	String         *params;
	Cardinal       *numb;
{
	if (appResources.justQuit) 
        Leave(0);
    else
        DialogPopup(toplevel, LeaveDialog, NULL, w);
}

/* ARGSUSED */
static void
LeaveDialog(pop, noop)
	Widget          pop;
	Cardinal        noop;
{
	Widget          di;

	/* Take "Really exit? from resources */
	di = XtCreateManagedWidget("exit", dialogWidgetClass, pop, NULL, 0);
	XawDialogAddButton(di, "yes", YesLeave, pop);
	XawDialogAddButton(di, "no", NoLeave, pop);
    XtOverrideTranslations(XtParent(di),
            XtParseTranslationTable("<Message>WM_PROTOCOLS: PressAButton(4)"));
    XtOverrideTranslations(di, XtParseTranslationTable(leaveTransl));
}


/* ARGSUSED */
static void
YesLeave(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	Leave(0);
}

/* ARGSUSED */
static void
NoLeave(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	XtDestroyWidget((Widget) closure);
}

/************************************************************************/
/* */
/* */
/* Deal with an attempt to double edit some data			 */
/* */
/* */
/************************************************************************/


void
NoEditIsPossible(w, da)
	Widget          w;
	Date           *da;
{
	static char     errmsg[256];

	FmtDate(da, errmsg, sizeof errmsg, appResources.already);

	w = DialogPopup(w, NoEdit, errmsg, w);
    XtOverrideTranslations(w, 
            XtParseTranslationTable("<Message>WM_PROTOCOLS: PopDownShell()"));
}

void
NoDayEditIsPossible(w, da)
	Widget          w;
	Date           *da;
{
	static char     errmsg[256];

	FmtDate(da, errmsg, sizeof errmsg, appResources.alreadyWeekly);

	DialogPopup(w, NoEdit, errmsg, w);

}

static void
NoEdit(pop, errmsg)
	Widget          pop;
	String          errmsg;
{
	Arg             args[2];
	Widget          dia;

	XtSetArg(args[0], XtNlabel, errmsg);
	dia = XtCreateManagedWidget("noedit", dialogWidgetClass, pop, args, 1);
	XawDialogAddButton(dia, "ok", NoCal, pop);
}
