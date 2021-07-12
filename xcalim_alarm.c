#include <X11/IntrinsicP.h> /* for toolkit stuff */
#include <X11/Intrinsic.h>
#include <X11/Xos.h>
#include <X11/StringDefs.h> /* for useful atom names */
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Paned.h>
#include <stdio.h>          /* for printing error messages */
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>       /* for stat() */
#include <pwd.h>            /* for getting username */
#include <errno.h>
#include "xcalim.h"

#ifndef wordBreak
#define wordBreak       0x01
#define scrollVertical  0x02
#endif

static char    *LocalMapStem;
static int     LocalMapStemMade;   /* day when LocalMapStem made last */
static struct  stat localstat;
static char    *TodayFile;
static struct  stat todaystat;
static Alarm   head;
static int     *countDown;
static int     countDownCt;

static XtCallbackRec callbacks[] = {
    {NULL, NULL},
    {NULL, NULL}
};

#define dprintf if (appResources.alarmScan) printf

#define MINUTES(hr, mn) ((hr)*60 + (mn))

/*
 * Local routines
 */
static void   AlarmScan();
static void   FreeAlarmList();
static void   setCall();
static void   AddAlarm();
static void   DisplayAlarmWindow();
static void   DestroyAlarm();
static void   HoldAlarm();
static void   AutoDestroy();
static void   AlarmEvent();
static void   ClockTick();
static void   AlarmsOff();
static int    readDataLine();
static String CreateCommand();
static int    ReloadAlarms();

/*
 * Initialise variables for the alarm mechanism
 */
void
InitAlarms()
{
    register char  *str;
    register char  *wk;
    register        ct;
    register int   *p;
    char           *XtMalloc();

    if (!MyCalendar)
        return;

    if (appResources.alarms == False) {
        dprintf("Alarms not enabled\n");
        return;
    }
    dprintf("Alarms on\n");
    /*
     * Interpret the countDown string established
     * by user - turn this into a vector
     * countDown -> points to a vector of integers
     *  indicating the number of mins each alarm will be set
     * countDownCt - the number of integers in the vector
     */
    if (str = appResources.countdown) {
        for (wk = str, ct = 0; *wk; wk++)
            if (*wk == ',')
                ct++;
        ct++;       /* no of things to store */

        countDown = (int *) XtMalloc(sizeof(int) * (ct + 2));

        p = countDown;
        while (*str)
            if (!isdigit(*str))
                str++;
            else
                break;
        while (*str) {
            *p = 0;
            ct = 0;
            while (isdigit(*str))
                ct = ct * 10 + *str++ - '0';
            *p++ = ct;
            countDownCt++;
            while (*str && !isdigit(*str))
                str++;
        }
    }
    if (appResources.update)
        ClockTick(0, 0);
    else
        AlarmFilePoll(0);
}


/*
 * ClockTick is polled every minute (update) to see if the data file has
 * altered - if so then a new data list is built
 */
static void
ClockTick(client_data, id)
    void *         client_data;
    XtIntervalId   *id;
{
    long            secs;
    time_t          ti;
    struct tm      *tm;


    ti = time(0);
    tm = localtime(&ti);

    dprintf("ClockTick %d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);

    secs = ClockSync(tm, appResources.update);
    (void) XtAppAddTimeOut(appContext, secs * 1000,
                (XtTimerCallbackProc) ClockTick,
                (void *) NULL);
    AlarmFilePoll(tm);
    MemoPoll();
}

void
AlarmFilePoll(tm)
    struct tm      *tm;
{
    time_t          ti;
    int     mnow;
    int             files, changed;
    struct stat oldstat;
    char           *home;
    char            buf[256];
    Alarm       *old;

    if (appResources.alarms == False)
        return;

    if (tm == NULL) {
        ti = time(0);
        tm = localtime(&ti);
    }
    /*
     * Create the name of the data file in a cache to save energy
     */
    if (LocalMapStemMade != tm->tm_mday) {
        LocalMapStemMade = tm->tm_mday;
        if (appResources.calCompat == False) {
            (void) sprintf(buf, "%s/xy%d/xc%d%s%d",
                       MapStem,
                       tm->tm_year + 1900,
                       tm->tm_mday,
                       appResources.smon[tm->tm_mon],
                       tm->tm_year + 1900);
        } else {
            (void) sprintf(buf, "%s/xc%d%s%d",
                       MapStem,
                       tm->tm_mday,
                       appResources.smon[tm->tm_mon],
                       tm->tm_year + 1900);
        }
        if (LocalMapStem)
            XtFree(LocalMapStem);
        LocalMapStem = XtNewString(buf);
        dprintf("Todays Daily Filename = %s\n", LocalMapStem);

        if (TodayFile)
            XtFree(TodayFile);

        TodayFile = XtNewString(MakeWeeklyName(tm->tm_wday));

        memset(&localstat, '\0', sizeof (struct stat));
        memset(&todaystat, '\0', sizeof (struct stat));
    }
    files = 0;
    changed = 0;
    /*
     * check for file existence
     */
    if (access(LocalMapStem, F_OK) == 0) {
        files = 1;
        oldstat = localstat;
        (void) stat(LocalMapStem, &localstat);
        if (oldstat.st_mtime != localstat.st_mtime ||
            oldstat.st_ctime != localstat.st_ctime)
            changed = 1;
    }
    if (access(TodayFile, F_OK) == 0) {
        files |= 2;
        oldstat = todaystat;
        (void) stat(TodayFile, &todaystat);
        if (oldstat.st_mtime != todaystat.st_mtime ||
            oldstat.st_ctime != todaystat.st_ctime)
            changed |= 2;
    }

    old = NULL;
    if (changed) {
        old = head.next;
        head.next = NULL;
    }
    if (files) {
        if (changed) {
            mnow = MINUTES(tm->tm_hour, tm->tm_min);
            if (files & 1)
                AlarmScan(LocalMapStem, tm, mnow);
            if (files & 2)
                AlarmScan(TodayFile, tm, mnow);
            /* check on any extant alarm */
            if (ReloadAlarms(old, mnow))
                AlarmsOff();
            if (appResources.interval_id == 0 && head.next)
                setCall();
        }
    } else
        dprintf("No files to scan\n");

    if (old)
        FreeAlarmList(old);

    UpdateMemo();
}

static int
ReloadAlarms(old, mnow)
    Alarm  *old;
    int     mnow;
{
    Alarm  *prev;

    dprintf("Checking old alarm details\n");
    if (head.next == NULL) {
        if (old) {
            dprintf("No new alarms, stop old running\n");
            return 1;
        }
    }
    else if (head.next->alarm_mm <= mnow &&
             (old == NULL ||
          old->alarm_mm > head.next->alarm_mm)) {
        /*
         * this looks different but let's be careful
         * if the entries on the new list are earlier
         * than those on the old - then we have
         * serviced those requests and we need to
         * ensure that we don't fire them again
         */
        while (head.next && head.next->alarm_mm <= mnow &&
               (old == NULL ||
                old->alarm_mm > head.next->alarm_mm)) {
            dprintf("Discard alarm\n");
            XtFree(head.next->alarm);
            XtFree(head.next->what);
            prev = head.next;
            head.next = prev->next;
            XtFree((char *)prev);
        }
        return (ReloadAlarms(old, mnow));
    }
    else if (old && old->alarm_mm != head.next->alarm_mm) {
        dprintf("First new alarm differs from old, stop old alarm\n");
        dprintf("Old: %d %s %s\n", old->alarm_mm, old->alarm, old->what);
        dprintf("New: %d %s %s\n", head.next->alarm_mm, head.next->alarm, head.next->what);
        return 1;
    }
    return 0;
}

static void
AlarmScan(file, tm, mnow)
    String          file;
    struct tm      *tm;
    int             mnow;
{
    FILE           *fp;
    char            hrstr[16];
    char            remline[256];
    char           *rem;
    int             hr, mn;
    Boolean         isAction;


    dprintf("Scanning data file %s\n", file);

    if ((fp = fopen(file, "r")) == NULL) {
        fprintf(stderr, "Unexpected failure to open: %s\n", file);
        exit(1);
    }
    while (readDataLine(fp, &hr, &mn, hrstr, remline)) {
        /* see if we have an action string to do */
        isAction = False;
        rem = remline;
        if (*rem == '!' || strncmp(remline, "%cron", 5) == 0) {
            if (appResources.execalarms == False)
                continue;
            isAction = True;
            if (*rem == '!')
                rem++;
            else    rem += 5;
            while (isspace(*rem))
                rem++;
            if (*rem == '\0')
                continue;
        }
        AddAlarm(mnow, isAction, hr, mn, hrstr, rem);
    }
    (void) fclose(fp);
}

/*
 * The idea here is to generate a sorted event list - one element for each
 * event. We will discard anything that has already happened
 */
static void
AddAlarm(mnow, exec, hr, mn, hrstr, rem)
    int             mnow;
    Boolean         exec;
    int             hr;
    int             mn;
    char           *hrstr;
    char           *rem;
{

    Alarm          *al, *prev, *new;
    char           *XtMalloc();
    int             al_hr, al_mn, mm;
    int             loop;
    int             zero = 0;
    int            *p;

    p = countDown;
    for (loop = countDownCt; loop--; p++) {
        al_hr = hr;
        al_mn = mn;
        if (*p) {
            al_mn -= *p;
            if (al_mn < 0) {
                al_mn += 60;
                al_hr--;
                if (al_hr < 0)
                    continue;
            }
        }
        if ((mm = MINUTES(al_hr, al_mn)) < mnow)
            continue;

        new = (Alarm *) XtMalloc(sizeof(Alarm));
        new->alarm = XtNewString(hrstr);
        new->what = XtNewString(rem);
        new->alarm_mm = mm;
        new->alarm_state = *p;
        new->isAction = exec;
        new->next = NULL;

        /* now insert into correct place in List */
        if (head.next == NULL) {
            head.next = new;
            dprintf("%s - %s; alarm at %02d:%02d\n",
                hrstr, rem, al_hr, al_mn);
        } else {
            for (prev = &head, al = head.next; al; prev = al, al = prev->next)
                if (mm < al->alarm_mm)
                    break;
            prev->next = new;
            new->next = al;
            dprintf("%s - %s; alarm at %02d:%02d\n",
                hrstr, rem, al_hr, al_mn);
        }
    }
}

/*
 * read a line looking for a time spec and some data
 * return 1 if found
 * return 0 at end
 * Time spec is: hhmm hmm
 * hh:mm hh.mm
 * h:mm h.mm
 * all above may be optionally followed by:
 * A a AM am Am aM P p PM pm Pm pM
 * the string is terminated by a space or a tab
 */
static int
readDataLine(fin, hrp, minp, timestr, remline)
    FILE *fin;
    int  *hrp;
    int  *minp;
    char *timestr;
    char *remline;
{
    register enum readState {
        Ignore, Hr1, Hr2, HrSep,
        Mn1, Mn2, AmPm, LoseM,
        LookSp, LoseSp, Store, AllDone
    }               state;
    register int    c = 0;
    int             hr = 0, mn = 0;
    char           *destp;
    Boolean         foundampm = False;

    if (feof(fin))
        return 0;

    state = Hr1;

    while (state != AllDone) {
        if ((c = getc(fin)) == EOF) {
            if (state == Store)
                break;
            return 0;
        }
        switch (state) {
        case Ignore:
            if (c == '\n')
                state = Hr1;
            continue;
        case Hr1:
            destp = timestr;
            if (isdigit(c)) {
                hr = c - '0';
                mn = 0;
                destp = timestr;
                *destp = '\0';
                state = Hr2;
                break;
            }
            state = (c == '\n' ? Hr1 : Ignore);
            continue;
        case Hr2:
            if (isdigit(c)) {
                hr = hr * 10 + c - '0';
                state = HrSep;
                break;
            }
            /* Falls through to .. */
        case HrSep:
            if (c == ':' || c == '.') {
                state = Mn1;
                break;
            }
            /* Falls through to .. */
        case Mn1:
            if (isdigit(c)) {
                mn = c - '0';
                state = Mn2;
                break;
            }
            /* Falls through to .. */
        case Mn2:
            if (isdigit(c)) {
                mn = mn * 10 + c - '0';
                state = AmPm;
                break;
            } else if (state == Mn2) {
                state = (c == '\n' ? Hr1 : Ignore);
                continue;
            }
            /* Falls through to .. */
        case AmPm:
            if (c == 'a' || c == 'A') {
                if (hr == 12)
                    hr = 0;
                foundampm = True;
                state = LoseM;
                break;
            } else if (c == 'p' || c == 'P') {
                if (hr < 12)
                    hr += 12;
                foundampm = True;
                state = LoseM;
                break;
            }
            /* Falls through to .. */
        case LoseM:
            if (c == 'M' || c == 'm') {
                state = LookSp;
                break;
            }
            /* Falls through to .. */
        case LookSp:
            if (c == ' ' || c == '\t') {
                state = LoseSp;
                if (hr >= 24 || mn >= 60)
                    state = Ignore;
                destp = remline;
                *destp = '\0';
                continue;
            } else {
                state = (c == '\n' ? Hr1 : Ignore);
                continue;
            }
            break;
        case LoseSp:
            if (c == ' ' || c == '\t')
                continue;
            state = Store;
            /* Falls through to .. */
        case Store:
            if (c == '\n') {
                state = AllDone;
                continue;
            }
            break;
        }
        *destp++ = c;
        *destp = '\0';
    }
    *hrp = hr;
    *minp = mn;
    return 1;
}

/*
 * set up the timeout for the next event
 */
static void
setCall()
{
    time_t          ti;
    long            mnow;
    struct tm      *tm;
    Alarm          *sc;
    int             togo;

    ti = time(0);
    tm = localtime(&ti);
    mnow = MINUTES(tm->tm_hour, tm->tm_min);

    for (sc = head.next; sc; sc = sc->next) {
        togo = sc->alarm_mm - mnow;
        if (togo == 0)
        {   AlarmEvent(1, 0);   /* don't miss any current alarms */
            sc = &head;
            continue;
        }
        if (togo < 0)
            continue;
        appResources.interval_id = XtAppAddTimeOut(appContext,
              togo * 60 * 1000 - tm->tm_sec * 1000,
              (XtTimerCallbackProc) AlarmEvent, (void *) NULL);
        dprintf("Alarm in %d mins (less %d secs)\n", togo, tm->tm_sec);
        break;
    }
}


static void
AlarmEvent(client_data, id)
    void *         client_data;
    XtIntervalId   *id;
{
    int             tnow;
    long            ti;
    struct tm      *tm;
    Alarm          *al, *action, *actionend;
    String          cmd;

    appResources.interval_id = 0;

    ti = time(0);
    tm = localtime(&ti);
    tnow = MINUTES(tm->tm_hour, tm->tm_min);

    dprintf("Alarm %d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);

    /*
     * Look for alarms that have fired and remove them
     * from the list
     */
    actionend = NULL;
    for (al = head.next; al; al = al->next) {
        if (al->alarm_mm <= tnow)
            actionend = al;
        else break;
    }
    if (actionend) {
        al = head.next;
        head.next = actionend->next;
        actionend->next = NULL;
        /*
         * now process the list
         */
        while (al) {
            if (tnow == al->alarm_mm) {
                if (appResources.cmd != NULL) {
                    cmd = CreateCommand(appResources.cmd, al->what);
                    system(cmd);
                    XtFree(cmd);
                }
                if (al->isAction == False) {
                    DisplayAlarmWindow(al->alarm_state, al->alarm, al->what);
                } else {
                    cmd = CreateCommand(al->what, NULL);
                    system(cmd);
                    XtFree(cmd);
                }
            }
            actionend = al->next;
            XtFree(al->alarm);
            XtFree(al->what);
            XtFree((char *) al);
            al = actionend;
        }
    }
    if (client_data == NULL)
        setCall();
}

static String
CreateCommand(cmd, arg)
    String  cmd;
    String  arg;
{
    String  buf;
    char    *s, *d;
    char    *fmt;
    int     cmdlen;
    int     arglen;

    cmdlen = strlen(cmd);
    arglen = arg ? strlen(arg) : 0;

    buf = XtMalloc(cmdlen + arglen + 200);
    if (arglen == 0) fmt = "%s&";
    else fmt = "%s '";
    (void) sprintf(buf, fmt, cmd);
    /*
     * copy the arguments worrying about any embedded quote
     */
#define QUOTE     '\''
#define BACKSLASH '\\'
    if (arglen) {
        d = buf + strlen(buf);
        for(s = arg; *s; s++) {
            if (*s == QUOTE) {
                *d++ = QUOTE;
                *d++ = BACKSLASH;
                *d++ = QUOTE;
            }
            *d++ = *s;
        }
        *d++ = QUOTE;
        *d++ = '&';
        *d++ = '\0';
    }
    return buf;
}

static void
FreeAlarmList(al)
    register Alarm *al;
{
    register Alarm *nx;

    for (; al; al = nx) {
        nx = al->next;
        XtFree(al->alarm);
        XtFree(al->what);
        XtFree((char *) al);
    }
}

static void
AlarmsOff()
{
    if (appResources.interval_id) {
        XtRemoveTimeOut(appResources.interval_id);
        appResources.interval_id = 0;
    }
}

typedef struct {
    Widget          sa_top;
    XtIntervalId    sa_id;
} AlarmStatus;

static void
DisplayAlarmWindow(tleft, str1, str2)
    int         tleft;
    String      str1, str2;
{
    Widget      shell, form, title, ah;
    char        *fmt;
    char        buf[255];
    AlarmStatus *als;

    /*
     * making the top level shell the override widget class causes it to
     * popup without window manager interaction or window manager
     * handles.  this also means that it pops on the foreground of an
     * xlocked terminal and is not resizable by the window manager.  If
     * any one finds that to be desirable behavior, then change the
     * transient class below to override.
     *
     * For now transient class is much better behaved
     */
    shell = XtVaCreatePopupShell("alarm", transientShellWidgetClass, toplevel,
            XtNallowShellResize, True,
            XtNinput, True,
            XtNsaveUnder, TRUE,
            NULL);

    form = XtVaCreateManagedWidget("alarmPanel", panedWidgetClass, shell, NULL);
    /*
     * create alarm status save area
     */
    als = (AlarmStatus *) XtMalloc(sizeof(AlarmStatus));

    als->sa_top = shell;
    als->sa_id = (XtIntervalId) NULL;
    if (appResources.autoquit) {
        als->sa_id = XtAppAddTimeOut(appContext,
                appResources.autoquit * 1000,
                AutoDestroy, (void *) als);
    }
    title = XtVaCreateManagedWidget("alarmForm", formWidgetClass, form,
            XtNshowGrip, False,
            XtNdefaultDistance, 2,
            NULL);

    /*
     * Hold button Take "Hold" from resources
     */
    callbacks[0].callback = HoldAlarm;
    callbacks[0].closure = (void *) als;
    ah = XtVaCreateManagedWidget("alarmHold", commandWidgetClass, title,
            XtNleft, XtChainLeft,
            XtNright, XtChainLeft,
            XtNcallback, callbacks,
            XtNsensitive, appResources.autoquit ? True : False,
            NULL);

    if (tleft == 0)
        fmt = appResources.alarmnow;
    else
        fmt = appResources.alarmleft;

    if (fmt && *fmt) {
        (void) sprintf(buf, fmt, tleft);
        (void) XtVaCreateManagedWidget("alarmTitle", labelWidgetClass, title,
                XtNfromHoriz, ah,
                XtNleft, XtChainLeft,
                XtNright, XtChainLeft,
                XtNborderWidth, 0,
                XtNfromVert, NULL,
                XtNvertDistance, 3,
                XtNlabel, buf,
                NULL);
    }
    /*
     * Now the text which is the remainder of the panel
     */
    (void) sprintf(buf, "%s\n%s\n", str1, str2);
    (void) XtVaCreateManagedWidget("alarmText", asciiTextWidgetClass, form,
            XtNshowGrip, False,
            XtNstring, buf,
            XtNdisplayCaret, False,
            NULL);

    XtOverrideTranslations(shell,
            XtParseTranslationTable("<Message>WM_PROTOCOLS: PopDownShell()"));
    XtPopup(shell, XtGrabNone);
    (void) XSetWMProtocols(XtDisplay(shell), XtWindow(shell), &delWin, 1);
}

/* ARGSUSED */
static void
DestroyAlarm(w, als, call_data)
    Widget          w;
    AlarmStatus    *als;
    void *         call_data;
{
    if (als->sa_id)
        XtRemoveTimeOut(als->sa_id);
    AutoDestroy(als, NULL);
}

/* ARGSUSED */
static void
HoldAlarm(w, als, call_data)
    Widget          w;
    AlarmStatus    *als;
    void *         call_data;
{
    if (als->sa_id)
        XtRemoveTimeOut(als->sa_id);
    XtSetSensitive(w, FALSE);
}

/* ARGSUSED */
static void
AutoDestroy(als, id)
    AlarmStatus    *als;
    XtIntervalId   *id;
{

    XtDestroyWidget(als->sa_top);
    XtFree((char *) als);
}
