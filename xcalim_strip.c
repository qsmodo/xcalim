/*** Deal with the calendar window and its buttons ***/
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/AsciiText.h>
#include "xcalim.h"

static XtCallbackRec callbacks[] = {
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL}
};
#define ClearCallbacks() memset((void *)callbacks, '\0', sizeof (callbacks))

Date            callb; //Contains date when calendar day button pressed


/* defTranslations are used to make the middle mouse button load a date file
from a primary selection when clicked on a strip */
static String   defTranslations =
"<Btn2Down>: set()\n\
<Btn2Up>: PrimaryInsert() unset()";
static String   weekTranslations =
"<Message>WM_PROTOCOLS: PopDownShell()\n\
<Key>1:                 WeekDay(0)\n\
<Key>2:                 WeekDay(1)\n\
<Key>3:                 WeekDay(2)\n\
<Key>4:                 WeekDay(3)\n\
<Key>5:                 WeekDay(4)\n\
<Key>6:                 WeekDay(5)\n\
<Key>7:                 WeekDay(6)";
static String   extTranslations =
"<Key>j:         PressAButton(6)\n\
<Key>k:          PressAButton(5)\n\
<Key>Left:       PressAButton(5)\n\
<Key>Right:      PressAButton(6)\n\
<Key>m:          PressAButton(7)\n\
<Key>c:          PressAButton(8)\n\
<Key>w:          PressAButton(12)\n\
<Key>h:          PressAButton(10)\n\
<Key>F1:         PressAButton(10)\n\
<Key>BackSpace:  Nr(-1)\n\
<Key>Escape:     Nr(-1)\n\
<Key>Return:     Nr(-2)\n\
<Key>0:          Nr(0)\n\
<Key>1:          Nr(1)\n\
<Key>2:          Nr(2)\n\
<Key>3:          Nr(3)\n\
<Key>4:          Nr(4)\n\
<Key>5:          Nr(5)\n\
<Key>6:          Nr(6)\n\
<Key>7:          Nr(7)\n\
<Key>8:          Nr(8)\n\
<Key>9:          Nr(9)";

/*
 * Forward routines local to this file
 */
static void     DayBack();
#ifndef LONG_IS_32_BITS
static void     YmBack();
#endif
static void     ResizeNicely();
void            Nr();
void            StripHelp();
void            PopDownShell();
void            WeeklyHelp();
void            MkDate();
void            CreateHeaderButtons();

/*
 * Local routines
 */
static void     LeftPadding(int, Widget *, Widget, Dimension);
static void     RightPadding(int, Widget *, Widget, Dimension, int);
static void     MakeNewMonth();
static void     PrimaryPaste();
static Cardinal DateSum();

#define argLD(N,V) { XtSetArg(args[nargs], N, V); nargs++; }

/* ARGSUSED */
void
DoWeekly(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	Date            thisday;

	thisday.day = 0;
	thisday.month = 0;
	thisday.year = 0;
	thisday.wday = 0;
	NewMonthStrip(&thisday, w, NULL);
}


/*
 * Start a strip calendar happening a callback of the > or < buttons in
 * another strip
 */
/* ARGSUSED */
static void
MakeNewMonth(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	Date            thisday;
    Widget          main, header, calendar;
    Arg             args[2];
	WidgetList      children;
    Cardinal        n;
    int             i;
    char            label[50];

    /* Destroy old calendar (paned widget), but not the header */
	XtVaGetValues(parent, XtNchildren, &children, XtNnumChildren, &n, NULL);
	for (i = 0; i < n; i++) {
		if (XtClass(children[i]) == panedWidgetClass)
            break;
    }
    XtDestroyWidget(children[i]);

	thisday.year = YrUnpack((Cardinal) closure);
	thisday.month = MoUnpack((Cardinal) closure);
	thisday.day = today.day;

    /* Reassign callback's closures to the back and next widgets */
    header = XtParent(w);
	callbacks[0].callback = MakeNewMonth;
    callbacks[0].closure = (void *) DateSum(&thisday, 1);
	XtVaSetValues(XtNameToWidget(header, "next"), XtNcallback, callbacks, NULL);
    callbacks[0].closure = (void *) DateSum(&thisday, -1);
	XtVaSetValues(XtNameToWidget(header, "back"), XtNcallback, callbacks, NULL);

    sprintf(label, "%s %d\n", appResources.mon[thisday.month], thisday.year);
    XtVaSetValues(XtNameToWidget(header, "month"), XtNlabel, label, NULL);

	NewMonthStrip(&thisday, NULL, header);
}

Dimension
NewMonthStrip(td, but, attach)
    Date       *td;
    Widget     but;
    Widget     attach;
{
    static XtTranslations but2;
    Widget     shell, mon, dw, lw, lwi, form[7] = {NULL}, monvp, mondt;
    char       iconName[256], dayNr[3];
    int        type, i, total = 0;
    MonthEntry *me;
    Instance   *ins;
    String     dayStr;
    Cardinal   thisDay, startLoop, numberOfDays;
    Boolean    defaultsAreSet = False, markThisMonth = False;
    Dimension  borderW = 1;

    type = (td->day == 0) ? ME_WEEKLY : ME_MONTHLY;

    /* There are lots of differences between Months and weekly strips here */
    switch (type) {
    case ME_MONTHLY:
        FmtDate(td, iconName, sizeof iconName, appResources.stripfmt);
        mon = XtVaCreateWidget(appResources.mon[td->month],
                panedWidgetClass, parent, 
                XtNfromVert, attach,
                XtNtop, XtChainTop,
                XtNbottom, XtChainBottom,
                NULL);
        ins = RegisterMonth(td->year, td->month, mon);
        thisDay = FirstDay(td->month, td->year);
        numberOfDays = NumberOfDays(td->month, td->year);
        startLoop = 1;
        /* * Get the map for this year */
        me = GetMonthEntry(td->year, td->month);
        /* * see if we will need to worry about marking today's entry */
        if (appResources.markToday && td->year == today.year && 
                td->month == today.month)
            markThisMonth = True;
        break;
    case ME_WEEKLY:
        shell = XtVaCreatePopupShell("weekly",
                topLevelShellWidgetClass, toplevel,
                XtNiconName, iconName,
                NULL);
        if (but && XtIsSubclass(but, commandWidgetClass))
            ButtonOff(but, shell);
        ins = RegisterMonth(0, 0, shell);
        mon = XtVaCreateManagedWidget(iconName, panedWidgetClass, shell, NULL);
        thisDay = 0;
        numberOfDays = 6;   /* test is <= */
        startLoop = 0;
        /* Get the map for this year */
        me = GetWeeklyEntry();
        /* See if we will need to worry about marking today's entry */
        if (appResources.markToday) markThisMonth = True;
        break;
    }
    
    /* * Create a Viewport, with a panel inside it */
    monvp = XtVaCreateManagedWidget("viewport", viewportWidgetClass, mon,
            XtNshowGrip, False,
            XtNallowVert, True,
            NULL);
    mondt = XtVaCreateManagedWidget("panel", formWidgetClass, monvp,
            XtNdefaultDistance, 0,
            NULL);

    LeftPadding(thisDay, form, mondt, borderW);

#ifdef	LONG_IS_32_BITS
	callbacks[0].callback = DayBack;
#else
	callbacks[0].callback = YmBack;
	callbacks[1].callback = DayBack;
#endif
	for (i = startLoop, total = thisDay; i <= numberOfDays; i++, total++) {
		dayStr = appResources.sday[thisDay];
#ifdef LONG_IS_32_BITS
		callbacks[0].closure = (void *) DatePack(thisDay, i, td->month, 
                td->year);
#else
		callbacks[0].closure = (void *) DatePack(td->month, td->year);
		callbacks[1].closure = (void *) DayPack(thisDay, i);
#endif

		/* Each square in the strip is form containing label - command */
        form[thisDay] = XtVaCreateManagedWidget(dayStr, formWidgetClass, mondt,
                XtNresizable, True,
                XtNdefaultDistance, 0,
                XtNfromHoriz, thisDay ? form[thisDay - 1] : NULL,
                XtNfromVert,  form[thisDay],
                XtNborderWidth, borderW,
                NULL);

        snprintf(dayNr, 3, "%02d", i);
		ins->i_day_label[i] = lw = XtVaCreateManagedWidget(dayNr,
                labelWidgetClass, form[thisDay],
                XtNlabel, type == ME_MONTHLY ? dayNr : dayStr,
                XtNborderWidth, 0,
                XtNtop, XtChainTop,
                XtNbottom, XtChainTop,
                XtNjustify, XtJustifyLeft,
                XtNwidth, appResources.squareW,
                XtNheight, appResources.labelH,
                XtNresizable, False,
                NULL);

		/* This puts text from the file should it exist for this day */
		ins->i_day_info[i] = lwi = XtVaCreateManagedWidget("info",
                commandWidgetClass, form[thisDay],
                XtNborderWidth, 0,
                XtNcallback, callbacks,
                XtNfromVert, lw,
                XtNtop, XtChainTop,
                XtNbottom, XtChainBottom,
                XtNjustify, XtJustifyLeft,
                XtNwidth, appResources.squareW,
                XtNheight, appResources.squareH,
                XtNinternalHeight, 2,
                XtNinternalWidth, 2,
                XtNresizable, False,
                XtNlabel, me->me_have[i] ? me->me_have[i] : "",
                NULL);

		/* To get a handle on the old values which are lost by
		 * highlighting we get them after we have created the widget.
		 * Then we highlight today. */
		if (markThisMonth &&
		    ((type == ME_MONTHLY && today.day == i) ||
		     (type == ME_WEEKLY && today.wday == i))) {
			XtVaGetValues(lw, 
                    XtNforeground, &ins->i_col.fg, 
                    XtNbackground, &ins->i_col.bg,
                    NULL);
			XtVaSetValues(lw, 
                    XtNforeground, appResources.today.fg, 
                    XtNbackground, appResources.today.bg, 
                    NULL);
			XtVaGetValues(lwi, 
                    XtNforeground, &ins->i_col.fg, 
                    XtNbackground, &ins->i_col.bg,
                    NULL);
			XtVaSetValues(lwi, 
                    XtNforeground, appResources.today.fg, 
                    XtNbackground, appResources.today.bg, 
                    NULL);
		}

		/* add translations */
		if (but2 == NULL) {
			but2 = XtParseTranslationTable(defTranslations);
		}
		XtOverrideTranslations(lwi, but2);

		thisDay = (thisDay + 1) % 7;

		/* * cope with 1752 */
		if (td->year == 1752 && td->month == 8 && i == 2) {
			i = 13;
			numberOfDays += 11;	/* giving back the 11 days */
		}
	}
	ClearCallbacks();
    switch (type) {
    case ME_MONTHLY:
        /* Create extra padding days if the month does not have six columns */
        RightPadding(thisDay, form, mondt, borderW, total);
        ResizeNicely(parent, attach, mon);
        XtManageChild(mon);
        XtOverrideTranslations(parent,
                XtParseTranslationTable(extTranslations));
        XtSetKeyboardFocus(parent, parent);
        return(appResources.squareW * 7 + borderW * 14);
    case ME_WEEKLY:
        XtPopup(shell, XtGrabNone);
        XtOverrideTranslations(shell, 
                XtParseTranslationTable(weekTranslations));
        XtSetKeyboardFocus(shell, shell);
        (void) XSetWMProtocols(XtDisplay(shell), XtWindow(shell), &delWin, 1);
        return(0);
    }
}

static void 
ResizeNicely(parent, header, mon)
    Widget      parent;
    Widget      header;
    Widget      mon;
{
    Dimension w, h, headerH;

    XtUnmanageChild(header);
    XtVaGetValues(parent, XtNwidth,  &w,       XtNheight, &h, NULL);
    XtVaGetValues(header, XtNheight, &headerH,                NULL);

    h -= headerH;
    if (w > 0 && h > 0) {
        XtVaSetValues(mon,    XtNwidth, w, XtNheight, h, NULL);
        XtVaSetValues(header, XtNwidth, w,               NULL);
    }
    XtManageChild(header);
}

/*
 * Get the height of the specified widget
 */
Dimension
wHeight(w)
	Widget	w;
{
	Arg		args[1];
	Dimension	H;

	XtSetArg(args[0], XtNheight, &H);
	XtGetValues(w, args, 1);
	return H;
}

/*
 * Create header bar for normal monthly strip
 */
void
CreateHeaderButtons(dw, td)
	Widget          dw;
	Date           *td;
{
	Widget          lw;
    char            label[256];
    Cardinal        width;
    int             max = 0, i, len;
    Pixel bg;

    for (i = 0; i < 12; i++) {
        len = strlen(appResources.mon[i]);
        if (len > max)
            max = len;
    }

    /* We don't want a frame around the time and year/month labels in the header
    bar, after all they are not buttons, but we still want they to be aligned to
    the buttons. So we need the frame, but it should be colored with the same
    color of the header's background. Here we fetch such color. */
    XtVaGetValues(dw, XtNbackground, &bg, NULL);

    sprintf(label, "%*s %d", max, appResources.mon[td->month], td->year);
    lw = XtVaCreateManagedWidget("month", labelWidgetClass, dw,
            XtNfromHoriz, NULL,
            XtNleft, XtChainLeft,
            XtNright, XtChainLeft,
            XtNjustify, XtJustifyRight,
            XtNborderWidth, 1,
            XtNborderColor, bg,
            XtNlabel, label,
            NULL);

    lw = XtVaCreateManagedWidget("time", labelWidgetClass, dw,
            XtNfromHoriz, lw,
            XtNleft, XtChainRight,
            XtNright, XtChainRight,
            XtNborderWidth, 1,
            XtNborderColor, bg,
            NULL);
    MkDate(lw);
    
	/* Help button label help from resources */
	if (appResources.giveHelp) {
		callbacks[0].callback = StripHelp;
		callbacks[0].closure = (void *) 0;
		lw = XtVaCreateManagedWidget("help", commandWidgetClass, dw,
                XtNleft, XtChainRight,               
                XtNright, XtChainRight,
                XtNcallback, callbacks,
                XtNfromHoriz, lw,
                NULL);
		ClearCallbacks();
	}

    /* Today button */
	callbacks[0].callback = MakeNewMonth;
	callbacks[0].closure = (void *) DateSum(&today, 0);
	lw = XtVaCreateManagedWidget("current", commandWidgetClass, dw,
            XtNcallback, (XtArgVal) callbacks,
            XtNfromHoriz, lw,
            XtNleft, XtChainRight,
            XtNright, XtChainRight,
            NULL);
	ClearCallbacks();

    /* Week button */
	callbacks[0].callback = DoWeekly;
	lw = XtVaCreateManagedWidget("weekly", commandWidgetClass, dw,
            XtNcallback, (XtArgVal) callbacks,
            XtNfromHoriz, lw,
            XtNleft, XtChainRight,
            XtNright, XtChainRight,
            NULL);
	ClearCallbacks();

    /* Memo button */
	callbacks[0].callback = DoMemo;
	lw = XtVaCreateManagedWidget("memo", commandWidgetClass, dw,
            XtNcallback, (XtArgVal) callbacks,
            XtNfromHoriz, lw,
            XtNleft, XtChainRight,
            XtNright, XtChainRight,
            NULL);
	ClearCallbacks();

	/*
	 * back one month label "<" from resources
	 */
	callbacks[0].callback = MakeNewMonth;
	callbacks[0].closure = (void *) DateSum(td, -1);
	lw = XtVaCreateManagedWidget("back", commandWidgetClass, dw,
	        XtNcallback, callbacks,
	        XtNfromHoriz, lw,
	        XtNleft, XtChainRight,
	        XtNright, XtChainRight,
            NULL);
	ClearCallbacks();

	/*
	 * On one month label ">" from resources
	 */
	callbacks[0].callback = MakeNewMonth;
	callbacks[0].closure = (void *) DateSum(td, 1);
	lw = XtVaCreateManagedWidget("next", commandWidgetClass, dw,
	        XtNcallback, callbacks,
	        XtNfromHoriz, lw,
	        XtNleft, XtChainRight,
	        XtNright, XtChainRight,
            NULL);
	ClearCallbacks();
}

/* Create padding days if the first day of the month is not sunday */
static void
LeftPadding(thisDay, form, mondt, borderW)
    int        thisDay;
    Widget     *form;
    Widget     mondt;
    Dimension  borderW;
{
    for (int i = 0; i < thisDay; i++) {
        form[i] = XtVaCreateManagedWidget("pad", formWidgetClass, mondt, 
                  XtNresizable, True,
                  XtNwidth, appResources.squareW,
                  XtNheight, appResources.squareH + appResources.labelH,
                  XtNborderWidth, borderW,
                  XtNfromHoriz, i ? form[i - 1] : NULL,
                  NULL);
    }
}

static void
RightPadding(thisDay, form, mondt, borderW, total)
    int        thisDay;
    Widget     *form;
    Widget     mondt;
    Dimension  borderW;
    int        total;
{
    for (; total <= 41; total++) {
        form[thisDay] = XtVaCreateManagedWidget("pad", 
                formWidgetClass, mondt,
                XtNresizable, True,
                XtNdefaultDistance, 0,
                XtNwidth, appResources.squareW,
                XtNheight, appResources.squareH + appResources.labelH,
                XtNfromHoriz, thisDay ? form[thisDay - 1] : NULL,
                XtNfromVert,  form[thisDay],
                XtNborderWidth, borderW,
                NULL);
        thisDay = (thisDay + 1) % 7;
    }
}

/* * Called when the date changes to ensure that the correct day has the
 * appropriate highlights */
void
ChangeHighlight(old, new)
	Date           *old;
	Date           *new;
{
	register Instance *ins;
	Arg             args[5];
	Cardinal        nargs;

	for (ins = FindInstanceList(old); ins; ins = ins->i_next) {
		nargs = 0;
		argLD(XtNforeground, ins->i_col.fg);
		argLD(XtNbackground, ins->i_col.bg);
		XtSetValues(ins->i_day_label[old->day], args, nargs);
		XtSetValues(ins->i_day_info[old->day], args, nargs);
	}

	for (ins = FindInstanceList(new); ins; ins = ins->i_next) {
		nargs = 0;
		argLD(XtNforeground, &ins->i_col.fg);
		argLD(XtNbackground, &ins->i_col.bg);
		XtGetValues(ins->i_day_label[new->day], args, nargs);
		XtGetValues(ins->i_day_info[new->day], args, nargs);

		nargs = 0;
		argLD(XtNforeground, appResources.today.fg);
		argLD(XtNbackground, appResources.today.bg);
		XtSetValues(ins->i_day_label[new->day], args, nargs);
		XtSetValues(ins->i_day_info[new->day], args, nargs);
	}
}

void 
PopUpMemo(w, event, params, numb)
        Widget        	w;
        XSelectionEvent *event;
        String         *params;
        Cardinal       *numb;
{
    if ((w = XtNameToWidget(w, "header.memo"))) 
        XtCallCallbacks(w, XtNcallback, NULL);
    else 
        fprintf(stderr, "xcalim: Could not find button memo.\n");
}

/* Handles 1..7 input on a week view */
void
WeekDay(w, event, params, numb)
        Widget        	w;
        XSelectionEvent *event;
        String         *params;
        Cardinal       *numb;
{
    char widgetName[256];
    int i = atoi(*params);
    sprintf(widgetName, "*%s.info", appResources.sday[i]);
    if ((w = XtNameToWidget(w, widgetName)))
        XtCallCallbacks(w, XtNcallback, NULL);
    else
        fprintf(stderr, "xcalim: Could not find widget for WeekDay action\n");
}

/* Scan number typed by user. If it was the first number, buffer it. Else,
combine it with the previous to form the whole number and open the corresponding
day edit window */
void
Nr(w, event, params, numb)
        Widget        	w;
        XSelectionEvent *event;
        String         *params;
        Cardinal       *numb;
{
    static Widget header, label, info;
    static int expect = 0;   //Are we expecting a 2nd key or is this the 1st?
    static int day;          //Day number under construction
    static Pixel fg, bg;     //Fore and background of the header
    int arg = atoi(*params);
    
    if (expect == 0) {
        if (arg >= 0) { //Number pressed
            /* Reverse header color when the user typed only the first number */
            header = XtNameToWidget(w, "header");
            XtVaGetValues(header, XtNbackground, &bg, XtNforeground, &fg, NULL);
            XtVaSetValues(header, XtNbackground, fg, XtNforeground, bg, NULL);
            day = arg;
            expect = 1;
        } else if (arg == -2) StartEditing(w, &today, NULL); //Enter: Edit today
        return;
    } else {
        XtVaSetValues(header, XtNbackground, bg, XtNforeground, fg, NULL);

        /* Number */
        if (arg >= 0) day = day * 10 + arg;
        else if (arg == -1) { /* Escape or BackSpace */
            day = 0;
            expect = 0;
            return;
        }
    }

    char dayStr[4];
    //The asterisk is because we want to find a grandchildren widget
    snprintf(dayStr, 4, "*%02d", day);
    if ((label = XtNameToWidget(w, dayStr))) {
        info  = XtNameToWidget(XtParent(label), "info");
        XtCallCallbacks(info, XtNcallback, NULL);
    }
    day = expect = 0;
}

/*
 * Called when middle mouse button is clicked on a date box
 * This gets the current selection and adds it to the file
 * corresponding to the day.
 * This allows quick data loading
 */
void
LoadFromPrimary(w, event, params, numb)
        Widget        	w;
        XSelectionEvent *event;
        String         *params;
        Cardinal       *numb;
{
	XtGetSelectionValue(w, XA_PRIMARY, XA_STRING, PrimaryPaste, 0, 
            XtLastTimestampProcessed(XtDisplay(w)));
}

static void
PrimaryPaste(w, xcd, sel, seltype, val, len, fmt)
	Widget		w;
	XtPointer	xcd;
	Atom		*sel;
	Atom		*seltype;
	XtPointer	val;
	unsigned long	*len;
	int		*fmt;	
{	
	String		s;
	int		n;
	Arg             args[1];
	Cardinal	v;
	XtCallbackRec	*cb;
	Date		da;

	/* deal with arguments to get the text */
	if (*seltype != XA_STRING)
		n = 0;
	else
		n = (*len) * (*fmt/8);
	if (n == 0)
		return;

	s = (String) XtMalloc(n+1);
	if (n > 0)
		memcpy(s, (char *)val, n);
	s[n] = 0;
	XtFree(val);

	/* get closure data to find the date */
	XtSetArg(args[0], XtNcallback, &cb);
	XtGetValues(w, args, 1);
	v = (Cardinal) cb->closure;
	da.month = MoUnpack(v),
	da.year = YrUnpack(v);
#ifndef LONG_IS_32_BITS
	cb++;
	v = (Cardinal) cb->closure;
#endif
	da.day = DyUnpack(v);
	da.wday = WdUnpack(v);
	/* Add text to day file (code in xcalim_edit.c) */
	AppendText(w, &da, s);
}


/* Delete window upon window manager close event. Can be reused by other
actions if they don't require resource cleanup. */
void
PopDownShell (w, event, params, numb)
	Widget          w;
	XEvent         *event;
	String         *params;
	Cardinal       *numb;
{
    XtPopdown(w);
    XtDestroyWidget(w);
}


/*
 * Month arithmetic and packing
 */
static          Cardinal
DateSum(td, inx)
	Date           *td;
	int             inx;
{
	int             m, y;

	m = td->month;
	y = td->year;
	m += inx;
	if (m < 0) {
		m = 11;
		y--;
	} else if (m > 11) {
		m = 0;
		y++;
	}
#ifdef LONG_IS_32_BITS
	return (DatePack(0, 0, m, y));
#else
	return (DatePack(m, y));
#endif
}

/*
 * Call back from day selection button press
 * This is done in two stages if cannot fold dates into a closure
 */
/* ARGSUSED */
static void
DayBack(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
#ifdef LONG_IS_32_BITS
	callb.month = MoUnpack((Cardinal) closure);
	callb.year = YrUnpack((Cardinal) closure);
#endif
	callb.day = DyUnpack((Cardinal) closure);
	callb.wday = WdUnpack((Cardinal) closure);
	StartEditing(w, &callb, NULL);
}

#ifndef LONG_IS_32_BITS
/* ARGSUSED */
static void
YmBack(w, closure, call_data)
	Widget          w;
	void *         closure;
	void *         call_data;
{
	callb.month = MoUnpack((Cardinal) closure);
	callb.year = YrUnpack((Cardinal) closure);
}
#endif
