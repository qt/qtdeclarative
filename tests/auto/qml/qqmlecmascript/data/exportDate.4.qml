import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    boolProperty: false

    Component.onCompleted: {
        var dt = datetimeExporter.getDateTime();
        var offset = datetimeExporter.getDateTimeOffset();
        var date = datetimeExporter.getDate();
        var timespec = datetimeExporter.getTimeSpec();

        // The test date is 2009-05-12 23:59:59 (UTC)
        var compare = new Date(Date.UTC(2009, 5-1, 12, 23, 59, 59));

        boolProperty = (dt.getTime() == compare.getTime() &&
                        offset == 0 &&
                        timespec == 'UTC' &&
                        dt.getUTCFullYear() == 2009 &&
                        dt.getUTCMonth() == 5-1 &&
                        dt.getUTCDate() == 12 &&
                        dt.getUTCHours() == 23 &&
                        dt.getUTCMinutes() == 59 &&
                        dt.getUTCSeconds() == 59 &&
                        date.getUTCFullYear() == 2009 &&
                        date.getUTCMonth() == 5-1 &&
                        date.getUTCDate() == 12);
    }
}
