import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        var dateVar = new Date("2009-05-12");
        var dateTimeEarly = new Date("2009-05-12T00:00:01");
        var dateTimeLate = new Date("2009-05-12T23:59:59");

        // Date, with no zone specified, is implicitly UTC, but is converted to QDate using LocalTime
        dateProperty = dateVar;
        // Date-time, with no zone, is implicitly local-time
        dateTimeProperty = dateTimeEarly;
        dateTimeProperty2 = dateTimeLate;

        var check = true;
        if (dateProperty.getTime() != dateVar.getTime()) {
            console.warn("Date epoch delta differs:",
                         dateProperty.getTime(), "!=", dateVar.getTime());
            check = false;
        }
        if (dateProperty.getUTCFullYear() != 2009) {
            console.warn("Date's year is wrong:",
                         dateProperty.getUTCFullYear(), "!= 2009");
            check = false;
        }
        if (dateProperty.getUTCMonth() != 5-1) {
            console.warn("Date's month is wrong:",
                         dateProperty.getUTCMonth(), "!= 4 (May)");
            check = false;
        }
        if (dateProperty.getUTCDate() != 12) {
            console.warn("Date's day is wrong:",
                         dateProperty.getUTCDate(), "!= 12");
            check = false;
        }
        if (dateProperty.getUTCHours() != 0) {
            console.warn("Date's hour is wrong:",
                         dateProperty.getUTCHours(), "!= 0");
            check = false;
        }
        if (dateProperty.getUTCMinutes() != 0) {
            console.warn("Date's minute is wrong:",
                         dateProperty.getUTCMinutes(), "!= 0");
            check = false;
        }
        if (dateProperty.getUTCSeconds() != 0) {
            console.warn("Date's second is wrong:",
                         dateProperty.getUTCSeconds(), "!= 0");
            check = false;
        }
        if (dateTimeProperty.getTime() != dateTimeEarly.getTime()) {
            console.warn("Early date-time epoch delta differs:",
                         dateTimeProperty.getTime(), "!=", dateTimeEarly.getTime());
            check = false;
        }
        if (dateTimeProperty.getFullYear() != 2009) {
            console.warn("Early date-time's year is wrong:",
                         dateTimeProperty.getFullYear(), "!= 2009");
            check = false;
        }
        if (dateTimeProperty.getMonth() != 5-1) {
            console.warn("Early date-time's month is wrong:",
                         dateTimeProperty.getMonth(), "!= 4 (May)");
            check = false;
        }
        if (dateTimeProperty.getDate() != 12) {
            console.warn("Early date-time's day is wrong:",
                         dateTimeProperty.getDate(), "!= 12");
            check = false;
        }
        if (dateTimeProperty.getHours() != 0) {
            console.warn("Early date-time's hour is wrong:",
                         dateTimeProperty.getHours(), "!= 0");
            check = false;
        }
        if (dateTimeProperty.getMinutes() != 0) {
            console.warn("Early date-time's minute is wrong:",
                         dateTimeProperty.getMinutes(), "!= 0");
            check = false;
        }
        if (dateTimeProperty.getSeconds() != 1) {
            console.warn("Early date-time's second is wrong:",
                         dateTimeProperty.getSeconds(), "!= 1");
            check = false;
        }
        if (dateTimeProperty2.getTime() != dateTimeLate.getTime()) {
            console.warn("Late date-time epoch delta differs:",
                         dateTimeProperty2.getTime(), "!=", dateTimeLate.getTime());
            check = false;
        }
        if (dateTimeProperty2.getFullYear() != 2009) {
            console.warn("Late date-time's year is wrong:",
                         dateTimeProperty2.getFullYear(), "!= 2009");
            check = false;
        }
        if (dateTimeProperty2.getMonth() != 5-1) {
            console.warn("Late date-time's month is wrong:",
                         dateTimeProperty2.getMonth(), "!= 4 (May)");
            check = false;
        }
        if (dateTimeProperty2.getDate() != 12) {
            console.warn("Late date-time's day is wrong:",
                         dateTimeProperty2.getDate(), "!= 12");
            check = false;
        }
        if (dateTimeProperty2.getHours() != 23) {
            console.warn("Late date-time's hour is wrong:",
                         dateTimeProperty2.getHours(), "!= 23");
            check = false;
        }
        if (dateTimeProperty2.getMinutes() != 59) {
            console.warn("Late date-time's minute is wrong:",
                         dateTimeProperty2.getMinutes(), "!= 59");
            check = false;
        }
        if (dateTimeProperty2.getSeconds() != 59) {
            console.warn("Late date-time's second is wrong:",
                         dateTimeProperty2.getSeconds(), "!= 59");
            check = false;
        }
        boolProperty = check;
    }
}
