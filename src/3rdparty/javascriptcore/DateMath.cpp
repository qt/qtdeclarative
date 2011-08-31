/*
 * Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.

 * Copyright 2006-2008 the V8 project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Google Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "DateMath.h"

#include <algorithm>
#include <limits.h>
#include <limits>
#include <stdint.h>
#include <time.h>
#include <math.h>

//#if HAVE(SYS_TIME_H)
#if defined(EXISTS_SYS_TIME)
#include <sys/time.h>
#endif

//#if HAVE(SYS_TIMEB_H)
#if defined(EXISTS_SYS_TIMEB)
#include <sys/timeb.h>
#endif

#define NaN std::numeric_limits<double>::quiet_NaN()

using namespace QV8DateConverter::WTF;

namespace QV8DateConverter {

namespace WTF {

/* Constants */

static const double minutesPerDay = 24.0 * 60.0;
static const double secondsPerDay = 24.0 * 60.0 * 60.0;
static const double secondsPerYear = 24.0 * 60.0 * 60.0 * 365.0;

static const double usecPerSec = 1000000.0;

static const double maxUnixTime = 2145859200.0; // 12/31/2037
// ECMAScript asks not to support for a date of which total
// millisecond value is larger than the following value.
// See 15.9.1.14 of ECMA-262 5th edition.
static const double maxECMAScriptTime = 8.64E15;

// Day of year for the first day of each month, where index 0 is January, and day 0 is January 1.
// First for non-leap years, then for leap years.
static const int firstDayOfMonth[2][12] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
};

static inline bool isLeapYear(int year)
{
    if (year % 4 != 0)
        return false;
    if (year % 400 == 0)
        return true;
    if (year % 100 == 0)
        return false;
    return true;
}

static inline int daysInYear(int year)
{
    return 365 + isLeapYear(year);
}

static inline double daysFrom1970ToYear(int year)
{
    // The Gregorian Calendar rules for leap years:
    // Every fourth year is a leap year.  2004, 2008, and 2012 are leap years.
    // However, every hundredth year is not a leap year.  1900 and 2100 are not leap years.
    // Every four hundred years, there's a leap year after all.  2000 and 2400 are leap years.

    static const int leapDaysBefore1971By4Rule = 1970 / 4;
    static const int excludedLeapDaysBefore1971By100Rule = 1970 / 100;
    static const int leapDaysBefore1971By400Rule = 1970 / 400;

    const double yearMinusOne = year - 1;
    const double yearsToAddBy4Rule = floor(yearMinusOne / 4.0) - leapDaysBefore1971By4Rule;
    const double yearsToExcludeBy100Rule = floor(yearMinusOne / 100.0) - excludedLeapDaysBefore1971By100Rule;
    const double yearsToAddBy400Rule = floor(yearMinusOne / 400.0) - leapDaysBefore1971By400Rule;

    return 365.0 * (year - 1970) + yearsToAddBy4Rule - yearsToExcludeBy100Rule + yearsToAddBy400Rule;
}

static inline double msToDays(double ms)
{
    return floor(ms / msPerDay);
}

int msToYear(double ms)
{
    int approxYear = static_cast<int>(floor(ms / (msPerDay * 365.2425)) + 1970);
    double msFromApproxYearTo1970 = msPerDay * daysFrom1970ToYear(approxYear);
    if (msFromApproxYearTo1970 > ms)
        return approxYear - 1;
    if (msFromApproxYearTo1970 + msPerDay * daysInYear(approxYear) <= ms)
        return approxYear + 1;
    return approxYear;
}

int dayInYear(double ms, int year)
{
    return static_cast<int>(msToDays(ms) - daysFrom1970ToYear(year));
}

static inline double msToMilliseconds(double ms)
{
    double result = fmod(ms, msPerDay);
    if (result < 0)
        result += msPerDay;
    return result;
}

// 0: Sunday, 1: Monday, etc.
static inline int msToWeekDay(double ms)
{
    int wd = (static_cast<int>(msToDays(ms)) + 4) % 7;
    if (wd < 0)
        wd += 7;
    return wd;
}

static inline int msToSeconds(double ms)
{
    double result = fmod(floor(ms / msPerSecond), secondsPerMinute);
    if (result < 0)
        result += secondsPerMinute;
    return static_cast<int>(result);
}

static inline int msToMinutes(double ms)
{
    double result = fmod(floor(ms / msPerMinute), minutesPerHour);
    if (result < 0)
        result += minutesPerHour;
    return static_cast<int>(result);
}

static inline int msToHours(double ms)
{
    double result = fmod(floor(ms/msPerHour), hoursPerDay);
    if (result < 0)
        result += hoursPerDay;
    return static_cast<int>(result);
}

int monthFromDayInYear(int dayInYear, bool leapYear)
{
    const int d = dayInYear;
    int step;

    if (d < (step = 31))
        return 0;
    step += (leapYear ? 29 : 28);
    if (d < step)
        return 1;
    if (d < (step += 31))
        return 2;
    if (d < (step += 30))
        return 3;
    if (d < (step += 31))
        return 4;
    if (d < (step += 30))
        return 5;
    if (d < (step += 31))
        return 6;
    if (d < (step += 31))
        return 7;
    if (d < (step += 30))
        return 8;
    if (d < (step += 31))
        return 9;
    if (d < (step += 30))
        return 10;
    return 11;
}

static inline bool checkMonth(int dayInYear, int& startDayOfThisMonth, int& startDayOfNextMonth, int daysInThisMonth)
{
    startDayOfThisMonth = startDayOfNextMonth;
    startDayOfNextMonth += daysInThisMonth;
    return (dayInYear <= startDayOfNextMonth);
}

int dayInMonthFromDayInYear(int dayInYear, bool leapYear)
{
    const int d = dayInYear;
    int step;
    int next = 30;

    if (d <= next)
        return d + 1;
    const int daysInFeb = (leapYear ? 29 : 28);
    if (checkMonth(d, step, next, daysInFeb))
        return d - step;
    if (checkMonth(d, step, next, 31))
        return d - step;
    if (checkMonth(d, step, next, 30))
        return d - step;
    if (checkMonth(d, step, next, 31))
        return d - step;
    if (checkMonth(d, step, next, 30))
        return d - step;
    if (checkMonth(d, step, next, 31))
        return d - step;
    if (checkMonth(d, step, next, 31))
        return d - step;
    if (checkMonth(d, step, next, 30))
        return d - step;
    if (checkMonth(d, step, next, 31))
        return d - step;
    if (checkMonth(d, step, next, 30))
        return d - step;
    step = next;
    return d - step;
}

static inline int monthToDayInYear(int month, bool isLeapYear)
{
    return firstDayOfMonth[isLeapYear][month];
}

static inline double timeToMS(double hour, double min, double sec, double ms)
{
    return (((hour * minutesPerHour + min) * secondsPerMinute + sec) * msPerSecond + ms);
}

double dateToDaysFrom1970(int year, int month, int day)
{
    year += month / 12;

    month %= 12;
    if (month < 0) {
        month += 12;
        --year;
    }

    double yearday = floor(daysFrom1970ToYear(year));
    int monthday = monthToDayInYear(month, isLeapYear(year));

    return yearday + monthday + day - 1;
}

static inline double ymdhmsToSeconds(long year, int mon, int day, int hour, int minute, int second)
{
    double days = (day - 32075)
        + floor(1461 * (year + 4800.0 + (mon - 14) / 12) / 4)
        + 367 * (mon - 2 - (mon - 14) / 12 * 12) / 12
        - floor(3 * ((year + 4900.0 + (mon - 14) / 12) / 100) / 4)
        - 2440588;
    return ((days * hoursPerDay + hour) * minutesPerHour + minute) * secondsPerMinute + second;
}

// We follow the recommendation of RFC 2822 to consider all
// obsolete time zones not listed here equivalent to "-0000".
static const struct KnownZone {
    const
        char tzName[4];
    int tzOffset;
} known_zones[] = {
    { "UT", 0 },
    { "GMT", 0 },
    { "EST", -300 },
    { "EDT", -240 },
    { "CST", -360 },
    { "CDT", -300 },
    { "MST", -420 },
    { "MDT", -360 },
    { "PST", -480 },
    { "PDT", -420 }
};

double timeClip(double t)
{
#if defined(_MSC_VER)
    if (!_finite(t)  || fabs(t) > maxECMAScriptTime)
        return  NaN;
    return t >= 0 ? floor(t) : ceil(t);
#else
    if (!isfinite(t) || fabs(t) > maxECMAScriptTime)
        return NaN;
    return trunc(t);
#endif
}
} // namespace WTF

namespace JSC {

double gregorianDateTimeToMS(const GregorianDateTime& t, double milliSeconds)
{
    double day = dateToDaysFrom1970(t.year + 1900, t.month, t.monthDay);
    double ms = timeToMS(t.hour, t.minute, t.second, milliSeconds);
    double result = (day * WTF::msPerDay) + ms;

    return result;
}

// input is UTC
void msToGregorianDateTime(double ms, GregorianDateTime& tm)
{
    const int year = msToYear(ms);
    tm.second   =  msToSeconds(ms);
    tm.minute   =  msToMinutes(ms);
    tm.hour     =  msToHours(ms);
    tm.weekDay  =  msToWeekDay(ms);
    tm.yearDay  =  dayInYear(ms, year);
    tm.monthDay =  dayInMonthFromDayInYear(tm.yearDay, isLeapYear(year));
    tm.month    =  monthFromDayInYear(tm.yearDay, isLeapYear(year));
    tm.year     =  year - 1900;
    tm.isDST    =  false;
    tm.utcOffset = static_cast<long>(0); // no ExecState :. cannot calculate offset.  Assume UTC output.
    tm.timeZone = NULL;
}

} // namespace JSC

} // namespace QV8DateConverter

