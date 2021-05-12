import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    boolProperty: false

    Component.onCompleted: {
        var dt = datetimeExporter.getDateTime();
        var offset = datetimeExporter.getDateTimeOffset();
        var date = datetimeExporter.getDate();
        var timespec = datetimeExporter.getTimeSpec();

        // The test date is 2009-05-12 00:00:01 (local time)
        var compare = new Date(2009, 5-1, 12, 0, 0, 1);
        var compareOffset = compare.getTimezoneOffset();

        boolProperty = (dt.getTime() == compare.getTime() &&
                        offset == compareOffset &&
                        timespec == 'LocalTime' &&
                        dt.getFullYear() == 2009 &&
                        dt.getMonth() == 5-1 &&
                        dt.getDate() == 12 &&
                        dt.getHours() == 0 &&
                        dt.getMinutes() == 0 &&
                        dt.getSeconds() == 1 &&
                        date.getUTCFullYear() == 2009 &&
                        date.getUTCMonth() == 5-1 &&
                        date.getUTCDate() == 12);
    }
}
