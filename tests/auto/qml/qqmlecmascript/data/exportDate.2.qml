import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    boolProperty: false

    Component.onCompleted: {
        var dt = datetimeExporter.getDateTime();
        var offset = datetimeExporter.getDateTimeOffset();
        var date = datetimeExporter.getDate();
        var timespec = datetimeExporter.getTimeSpec();

        // The test date is 2009-05-12 23:59:59 (local time)
        var compare = new Date(2009, 5-1, 12, 23, 59, 59);
        var compareOffset = compare.getTimezoneOffset();

        boolProperty = (dt.getTime() == compare.getTime() &&
                        offset == compareOffset &&
                        timespec == 'LocalTime' &&
                        dt.getFullYear() == 2009 &&
                        dt.getMonth() == 5-1 &&
                        dt.getDate() == 12 &&
                        dt.getHours() == 23 &&
                        dt.getMinutes() == 59 &&
                        dt.getSeconds() == 59 &&
                        date.getUTCFullYear() == 2009 &&
                        date.getUTCMonth() == 5-1 &&
                        date.getUTCDate() == 12);
    }
}
