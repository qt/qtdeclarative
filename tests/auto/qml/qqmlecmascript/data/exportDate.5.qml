import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    boolProperty: false

    Component.onCompleted: {
        var dt = datetimeExporter.getDateTime();
        var offset = datetimeExporter.getDateTimeOffset();
        var date = datetimeExporter.getDate();
        var timespec = datetimeExporter.getTimeSpec();

        // The test date is 2009-05-12 00:00:01 (UTC+11:30)
        var compare = new Date('2009-05-12T00:00:01+11:30');
        // That's 2009-05-11 12:30:01 UTC

        boolProperty = (dt.getTime() == compare.getTime() &&
                        offset == 11 * 60 + 30 &&
                        timespec == '+11:30' &&
                        dt.getUTCFullYear() == 2009 &&
                        dt.getUTCMonth() == 5-1 &&
                        dt.getUTCDate() == 11 &&
                        dt.getUTCHours() == 12 &&
                        dt.getUTCMinutes() == 30 &&
                        dt.getUTCSeconds() == 1 &&
                        date.getUTCFullYear() == 2009 &&
                        date.getUTCMonth() == 5-1 &&
                        date.getUTCDate() == 12);
    }
}
