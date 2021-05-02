/*** Generate help screens in separate popup windows ***/
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Paned.h>
#include "xcalim.h"

static XtCallbackRec callbacks[] = {
    {NULL, NULL},
    {NULL, NULL}
};

static  Boolean helpRead = False;

typedef struct helpdata {
    String  name;       /* Help name */
    String  text;       /* the text we are showing */
    Widget  popup;      /* Widget value */
} Help;

static Help help[] = {
    { "nohelp", "Sadly, Xcalim help has not been installed correctly\n", },
    { "main", },        /* don't move from here - see PopdownHelp */
    { "edit", },
    { "memo", },
    { "weekly", },
    { "strip", },
    { NULL, }
};

static Help    *findHelp();
static void	initHelp();
static char    *connectHelp();
static Widget   DisplayHelpWindow();
       void	PopdownHelp();
static void	makeHelpPopup();

/*
 *	Initialise the help system
 *	Two methods - from a help file
 *	compiled in
 */
char	*helpText;
/* defines helpdata[] */
#include "xcalim_help.h"

/*
 * Search the help structure looking for a name
 */
static Help *
findHelp(name)
    String name;
{
    Help *h;

    for (h = help; h->name; h++)
        if (strcmp(name, h->name) == 0)
            return h;
    return NULL;
}

/*
 * Initialise help data
 */
static void
initHelp()
{
    Help    *he;
    char    *h;
    char    *name;
    char    *text;
    int     lastc;

    /*
     * Find the text for the help data
     * If take from a file - then connect the file in memory
     * If that fails, then use the text compiled into the program
     */
    if (appResources.helpFromFile == True) {
        helpText = connectHelp();
        if (helpText == NULL)
            helpText = helpdata;
    } else
        helpText = helpdata;
    /*
     * Allow you to compile NO text into the program
     */
    if (*helpText == '\0')
        helpText = NULL;
    /*
     * helpRead says - I have TRIED to find the data
     */
    helpRead = True;
    if (helpText == NULL)
        return;

    for (h = helpText; *h; ) {
        if ((h = strchr(h, '{')) == NULL)
            break;
        h++;            /* h points to name */
        name = h;

        /* look for the end of the name */
        while (!isspace(*h))    /* name is terminated by white space */
            h++;        /* or a newline */
        lastc = *h;
        *h++ = '\0';
        if (lastc != '\n') {
            while(isspace(*h))
                h++;
        }
        text = h;       /* that's the start of the text */

        /* and the end of the text */
        h = strchr(h, '}');
        if (h == NULL)
            Fatal("Missing } in help text - %s\n", name);
        *h++ = '\0';

         /* insert the details in the structure */
        if (he = findHelp(name))
            he->text = text;
    }
}

/*
 * If we are getting help from a file then the name is in the
 * resources. The trick here is to map the file into memory privately
 * so that it is just like one compiled in vector
 */
static char *
connectHelp()
{
        int     fd;
        struct  stat statb;
        char    *fibase;

        if ((fd = open(appResources.helpfile, 0)) < 0)
                return NULL;

        if (fstat(fd, &statb) < 0) {
                printf("Cannot fstat %s (shouldn't happen)\n", appResources.helpfile);
                close(fd);
                return NULL;
        }

        if (statb.st_size == 0) {
                printf("Zero length file? (%s)\n", appResources.helpfile);
                close(fd);
                return NULL;
        }
        fibase = (char *) mmap(0, statb.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
        if ((int)fibase == -1) {
                printf("Cannot map %s into memory\n", appResources.helpfile);
                return NULL;
        }
        close(fd);      /* we have it now */
        fibase[statb.st_size] = '\0'; /* we don't mind losing the last char */
        return(fibase);
}

static Widget
DisplayHelpWindow(str)
    String          str;
{
    Widget          shell, form, title;

    shell = XtVaCreatePopupShell("help", topLevelShellWidgetClass, toplevel,
            NULL);

    (void) XtVaCreateManagedWidget("helpText", asciiTextWidgetClass, shell,
            XtNshowGrip, False,
            XtNstring, str,
            XtNdisplayCaret, False,
            NULL);

    XtOverrideTranslations(shell,
            XtParseTranslationTable("<Message>WM_PROTOCOLS: CloseHelp()"));
    XtPopup(shell, XtGrabNone);
    (void) XSetWMProtocols(XtDisplay(shell), XtWindow(shell), &delWin, 1);
    return shell;
}

/* ARGSUSED */
void
PopdownHelp(w, closure, call_data)
    Widget  w;
    void   *closure;
    void   *call_data;
{
    XtPopdown(w);
    HelpButtonOn(w);
}

/*
 * Create a Help Popup
 */
static void
makeHelpPopup(w, name)
    Widget w;
    char   *name;
{
    Help   *he;

    if (helpRead == False)
        initHelp();

    he = findHelp(name);
    if (he == NULL || he->text == NULL)
        he->text = help[0].text;

    if (he->popup) XtPopup(he->popup, XtGrabNone);
    else           he->popup = DisplayHelpWindow(he->text);

    ButtonOff(w, he->popup);
}

/* ARGSUSED */
void
StripHelp(w, closure, call_data)
    Widget w;
    void   *closure;
    void   *call_data;
{
    makeHelpPopup(w, "strip");
}

/* ARGSUSED */
void
EditHelp(w, closure, call_data)
    Widget          w;
    void *         closure;
    void *         call_data;
{
    makeHelpPopup(w, "edit");
}

/* ARGSUSED */
void
MemoHelp(w, closure, call_data)
    Widget          w;
    void *         closure;
    void *         call_data;
{
    makeHelpPopup(w, "memo");
}

/* ARGSUSED */
void
WeeklyHelp(w, closure, call_data)
    Widget          w;
    void *         closure;
    void *         call_data;
{
    makeHelpPopup(w, "weekly");
}
