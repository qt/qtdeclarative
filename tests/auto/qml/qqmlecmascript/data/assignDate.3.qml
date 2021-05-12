import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    dateProperty: if (1) "2009-05-12"
    dateTimeProperty: if (1) "2009-05-12T00:00:01Z"
    dateTimeProperty2: if (1) "2009-05-12T23:59:59Z"

    boolProperty: false
    Component.onCompleted: {
        var dateVar = new Date("2009-05-12");
        var dateTimeEarly = new Date("2009-05-12T00:00:01Z");
        var dateTimeLate = new Date("2009-05-12T23:59:59Z");

        boolProperty = (dateProperty.getTime() == dateVar.getTime() &&
                        dateProperty.getUTCFullYear() == 2009 &&
                        dateProperty.getUTCMonth() == 5-1 &&
                        dateProperty.getUTCDate() == 12 &&
                        dateProperty.getUTCHours() == 0 &&
                        dateProperty.getUTCMinutes() == 0 &&
                        dateProperty.getUTCSeconds() == 0 &&
                        dateTimeProperty.getTime() == dateTimeEarly.getTime() &&
                        dateTimeProperty.getUTCFullYear() == 2009 &&
                        dateTimeProperty.getUTCMonth() == 5-1 &&
                        dateTimeProperty.getUTCDate() == 12 &&
                        dateTimeProperty.getUTCHours() == 0 &&
                        dateTimeProperty.getUTCMinutes() == 0 &&
                        dateTimeProperty.getUTCSeconds() == 1 &&
                        dateTimeProperty2.getTime() == dateTimeLate.getTime() &&
                        dateTimeProperty2.getUTCFullYear() == 2009 &&
                        dateTimeProperty2.getUTCMonth() == 5-1 &&
                        dateTimeProperty2.getUTCDate() == 12 &&
                        dateTimeProperty2.getUTCHours() == 23 &&
                        dateTimeProperty2.getUTCMinutes() == 59 &&
                        dateTimeProperty2.getUTCSeconds() == 59);
    }
}
