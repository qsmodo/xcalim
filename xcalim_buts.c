/***
  Deal with callback management for sensitive buttons
  When a button is pressed we make it insensitive.
  In general a button starts a popup - when the popup
  dies we reset the button.
  Problem is when the button dies before the popup.
***/
#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <X11/Intrinsic.h>
#include <X11/Xos.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>
#include "xcalim.h"

typedef struct active {
    struct active *next;
    Widget button;
    Widget popup;
} Active;

static Active *act;

/*
 * Local routines
 */
static void AddToActive();
static Active *LookActive();
static Active *LookPopup();
static void DeleteFromActive();
static void PopupIsDead();
static void ButtonIsDead();

/*
 * Here's the global entry point
 * given a button and a popup
 * make the button insensitive
 * add a destroy callback to the popup to turn the button on again when
 * the popup dies - assuming the button is in the list
 * add a destroy callback to the button to remove the button from the list
 */
void
ButtonOff(but, popup)
    Widget but;
    Widget popup;
{
    XtSetSensitive(but, False);
    XtAddCallback(popup, XtNdestroyCallback, PopupIsDead, but);
    XtAddCallback(but, XtNdestroyCallback, ButtonIsDead, but);
    AddToActive(but, popup);
}

/* * Turn a button on */
void
ButtonOn(but)
    Widget but;
{
    Active *ap;

    if (ap = LookActive(but)) {
        XtRemoveCallback(but, XtNdestroyCallback, ButtonIsDead, but);
        XtSetSensitive(but, True);
        DeleteFromActive(but);
    }
}

/* * Turn a help button on */
void
HelpButtonOn(popup)
    Widget popup;
{
    Active *ap;

    while (ap = LookPopup(popup))
        ButtonOn(ap->button);
}

/*
 * Callbacks
 * 1) called from popup when it dies
 */
/* ARGSUSED */
static void
PopupIsDead(w, closure, call_data)
    Widget w;
    void   *closure;
    void   *call_data;
{
    ButtonOn((Widget) closure);
}

/*
 * Callbacks
 * 2) called when button is dead
 */
/* ARGSUSED */
static void
ButtonIsDead(w, closure, call_data)
    Widget w;
    void   *closure;
    void   *call_data;

{
    Active *ap;
    Widget but = (Widget)closure;

    if (ap = LookActive(but)) {
        XtRemoveCallback(ap->popup, XtNdestroyCallback, PopupIsDead, but);
        DeleteFromActive(but);
    }
}

/* * Add a button to the active list */
    static void
AddToActive(but, popup)
    Widget but;
    Widget popup;
{
    Active *ap;

    if (LookActive(but) == NULL) {
        ap = (Active *) XtMalloc(sizeof (Active));
        ap->button = but;
        ap->popup = popup;
        ap->next = act;
        act = ap;
    } /* not convinced that the else arm here is a never happen */
}

    static Active *
LookActive(but)
    Widget but;
{
    Active *ap;

    for (ap = act; ap; ap = ap->next) {
        if (ap->button == but)
            return ap;
    }
    return NULL;
}

static Active *
LookPopup(popup)
    Widget popup;
{
    Active *ap;

    for (ap = act; ap; ap = ap->next) {
        if (ap->popup == popup)
            return ap;
    }
    return NULL;
}

/* * remove an active entry */
static void
DeleteFromActive(but)
    Widget but;
{
    Active *ap;
    Active *lp = NULL;

    for (ap = act; ap; ap = ap->next) {
        if (ap->button == but) {
            if (lp == NULL) 
                act = ap->next;
            else
                lp->next = ap->next;
            XtFree((char *)ap);
            return;
        }
        lp = ap;
    }
}
