import QtQuick 2.0
import Qt.test 1.0

Item {
    MyDateClass {
        id: mdc
    }

    function test_is_invalid_qtDateTime()
    {
        var dt = mdc.invalidDate();
        return isNaN(dt);
    }

    function test_is_invalid_jsDateTime()
    {
        var dt = new Date("");
        return isNaN(dt);
    }

    function test_rfc2822_date()
    {
        var dt = new Date("Wed, 18 Sep 2013 07:00:51 -0700");
        return dt.getTime();
    }

    function check_value(date, tag, qdt) {
        var result = true;
        if (date.getFullYear() != 2014) {
            console.warn("Wrong year (" + tag + "):", date.getFullYear(), "!= 2014")
            result = false;
        }
        // July; JS's months are Jan 0 to 11 Dec, vs. Qt's 1 to 12.
        if (date.getMonth() != 6) {
            console.warn("Wrong month (" + tag + "):", date.getMonth(), "!= 6");
            result = false;
        }
        if (date.getDate() != 16) {
            console.warn("Wrong day (" + tag + "):", date.getDate(), "!= 16");
            result = false;
        }
        if (date.getHours() != 23) {
            console.warn("Wrong hour (" + tag + "):", date.getHours(), "!= 23");
            result = false;
        }
        if (date.getMinutes() != 30) {
            console.warn("Wrong minute (" + tag + "):", date.getMinutes(), "!= 30");
            result = false;
        }
        if (date.getSeconds() != 31) {
            console.warn("Wrong second (" + tag + "):", date.getSecondss(), "!= 31");
            result = false;
        }

        if (qdt != undefined) {
            if (date.toISOString() != qdt.toISOString()) {
                console.warn("Different ISO strings (" + tag + "):",
                             date.toISOString(), "!=", qdt.toISOString);
                result = false;
            }
            if (date.getTime() != qdt.getTime()) {
                console.warn("Different epoch times (" + tag + "):",
                             date.getTime(), "!=", qdt.getTime());
            }
        }
        return result;
    }

    function check_date(qdt) {
        var result = check_value(qdt, "passed");
        if (!check_value(new Date(2014, 6, 16, 23, 30, 31), "construct", qdt))
            result = false;
        if (!check_value(new Date(Date.parse("2014-07-16T23:30:31")), "parsed", qdt))
            result = false;

        var date = new Date(0);
        date.setFullYear(2014);
        date.setMonth(6);
        date.setDate(16);
        date.setHours(23);
        date.setMinutes(30);
        date.setSeconds(31);
        if (!check_value(date, "by field", qdt))
            result = false;

        return result;
    }
}
