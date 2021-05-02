/***
  Provide two routines
  NumberOfDays(month, year): the number of days in a Month and Year
  FirstDay(month, year): the day a month starts on
***/

#include <X11/Intrinsic.h>

/*
 * This notice and the string below must not be removed from this code
 */

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kim Letkeman.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define THURSDAY 4  /* for reformation */
#define SATURDAY 6  /* 1 Jan 1 was a Saturday */

#define FIRST_MISSING_DAY   639787  /* 3 Sep 1752 */
#define NUMBER_MISSING_DAYS 11      /* 11 day correction */

static int days_in_month[2][13] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

/* leap year -- account for gregorian reformation in 1752 */
#define leap_year(yr) \
    ((yr) <= 1752 ? !((yr) % 4) : \
    !((yr) % 4) && ((yr) % 100) || !((yr) % 400))

/* number of centuries since 1700, not inclusive */
#define centuries_since_1700(yr) \
    ((yr) > 1700 ? (yr) / 100 - 17 : 0)

/* number of centuries since 1700 whose modulo of 400 is 0 */
#define quad_centuries_since_1700(yr) \
    ((yr) > 1600 ? ((yr) - 1600) / 400 : 0)

/* number of leap years between year 1 and this year, not inclusive */
#define leap_years_since_year_1(yr) \
    ((yr) / 4 - centuries_since_1700(yr) + quad_centuries_since_1700(yr))

static int day_in_year();
static int day_in_week();

Cardinal
FirstDay(month, year)
    int month;
    int year;
{
    /* Months in xcalim are 0 - 11 */
    if (month == 8 && year == 1752)  /* this should work but doesn't */
        return 2;
    return day_in_week(1, month, year);
}

Cardinal
NumberOfDays(month, year)
    int month;
    int year;
{
    /* Months in xcalim are 0 - 11 */
    if (month == 8 && year == 1752)
        return 19;
    return days_in_month[leap_year(year)][month];
}


/* * return the 1 based day number within the year */
static int
day_in_year(day, month, year)
    register int day, month;
    int year;
{
    register int i, leap;

    leap = leap_year(year);
    for (i = 0; i < month; i++)
        day += days_in_month[leap][i];
    return(day);
}

/*
 *  return the 0 based day number for any date from 1 Jan. 1 to
 *  31 Dec. 9999.  Assumes the Gregorian reformation eliminates
 *  3 Sep. 1752 through 13 Sep. 1752.  Returns Thursday for all
 *  missing days.
 */
static int
day_in_week(day, month, year)
    int day, month, year;
{
    long temp;

    temp = (long)(year - 1) * 365 + leap_years_since_year_1(year - 1)
        + day_in_year(day, month, year);
    if (temp < FIRST_MISSING_DAY)
        return((temp - 1 + SATURDAY) % 7);
    if (temp >= (FIRST_MISSING_DAY + NUMBER_MISSING_DAYS))
        return(((temp - 1 + SATURDAY) - NUMBER_MISSING_DAYS) % 7);
    return(THURSDAY);
}
