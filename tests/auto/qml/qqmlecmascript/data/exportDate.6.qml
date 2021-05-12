import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    boolProperty: false

    Component.onCompleted: {
        var dt = datetimeExporter.getDateTime();
        var offset = datetimeExporter.getDateTimeOffset();
        var date = datetimeExporter.getDate();
        var timespec = datetimeExporter.getTimeSpec();

        // The test date is 2009-05-12 23:59:59 (UTC+11:30)
        var compare = new Date('2009-05-12T23:59:59+11:30');
        // That's 2009-05-12 12:29:59 UTC

        boolProperty = (dt.getTime() == compare.getTime() &&
                        offset == 11 * 60 + 30 &&
                        timespec == '+11:30' &&
                        dt.getUTCFullYear() == 2009 &&
                        dt.getUTCMonth() == 5-1 &&
                        dt.getUTCDate() == 12 &&
                        dt.getUTCHours() == 12 &&
                        dt.getUTCMinutes() == 29 &&
                        dt.getUTCSeconds() == 59 &&
                        date.getUTCFullYear() == 2009 &&
                        date.getUTCMonth() == 5-1 &&
                        date.getUTCDate() == 12);
    }
}
