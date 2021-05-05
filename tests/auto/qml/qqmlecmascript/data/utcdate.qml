import QtQuick 2.0
import Qt.test 1.0

Item {
    function check_value(date, tag, qdt) {
        var result = true;
        if (date.getUTCFullYear() != 2014) {
            console.warn("Wrong year (" + tag + "):", date.getUTCFullYear(), "!= 2014")
            result = false;
        }
        // July; JS's months are Jan 0 to 11 Dec, vs. Qt's 1 to 12.
        if (date.getUTCMonth() != 6) {
            console.warn("Wrong month (" + tag + "):", date.getUTCMonth(), "!= 6");
            result = false;
        }
        if (date.getUTCDate() != 16) {
            console.warn("Wrong day (" + tag + "):", date.getUTCDate(), "!= 16");
            result = false;
        }
        if (date.getUTCHours() != 23) {
            console.warn("Wrong hour (" + tag + "):", date.getUTCHours(), "!= 23");
            result = false;
        }
        if (date.getUTCMinutes() != 30) {
            console.warn("Wrong minute (" + tag + "):", date.getUTCMinutes(), "!= 30");
            result = false;
        }
        if (date.getUTCSeconds() != 31) {
            console.warn("Wrong second (" + tag + "):", date.getUTCSecondss(), "!= 31");
            result = false;
        }

        if (qdt != undefined) {
            if (date.toISOString() != qdt.toISOString()) {
                console.warn("Different ISO strings (" + tag + "):",
                             date.toISOString(), "!=", qdt.toISOString());
                result = false;
            }
            if (date.getTime() != qdt.getTime()) {
                console.warn("Different epoch times (" + tag + "):",
                             date.getTime(), "!=", qdt.getTime());
            }
        }
        return result;
    }

    function check_utc(qdt) {
        var result = check_value(qdt, "raw");
        if (!check_value(new Date(Date.UTC(2014, 6, 16, 23, 30, 31)), "by args", qdt))
            result = false;
        if (!check_value(new Date(Date.parse("2014-07-16T23:30:31Z")), "parsed", qdt))
            result = false;

        var utcDate = new Date(0);
        utcDate.setUTCFullYear(2014);
        utcDate.setUTCMonth(6);
        utcDate.setUTCDate(16);
        utcDate.setUTCHours(23);
        utcDate.setUTCMinutes(30);
        utcDate.setUTCSeconds(31);
        if (!check_value(utcDate, "by field", qdt))
            result = false;

        return result;
    }
}
