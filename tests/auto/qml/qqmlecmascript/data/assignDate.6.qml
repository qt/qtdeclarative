import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    dateProperty: if (1) new Date("2009-05-12")
    dateTimeProperty: if (1) new Date("2009-05-12T00:00:01+02:00")
    dateTimeProperty2: if (1) new Date("2009-05-12T23:59:59+02:00")

    boolProperty: false
    Component.onCompleted: {
        var dateVar = new Date("2009-05-12");
        var dateTimeEarly = new Date("2009-05-12T00:00:01+02:00");
        var dateTimeLate = new Date("2009-05-12T23:59:59+02:00");

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
                        dateTimeProperty.getUTCDate() == 11 &&
                        dateTimeProperty.getUTCHours() == 22 &&
                        dateTimeProperty.getUTCMinutes() == 0 &&
                        dateTimeProperty.getUTCSeconds() == 1 &&
                        dateTimeProperty2.getTime() == dateTimeLate.getTime() &&
                        dateTimeProperty2.getUTCFullYear() == 2009 &&
                        dateTimeProperty2.getUTCMonth() == 5-1 &&
                        dateTimeProperty2.getUTCDate() == 12 &&
                        dateTimeProperty2.getUTCHours() == 21 &&
                        dateTimeProperty2.getUTCMinutes() == 59 &&
                        dateTimeProperty2.getUTCSeconds() == 59);
    }
}
