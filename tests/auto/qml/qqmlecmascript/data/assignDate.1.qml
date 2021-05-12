import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        var dateVar = new Date(Date.UTC(2009, 4, 12));
        var dateTimeEarly = new Date(Date.UTC(2009, 4, 12, 0, 0, 1));
        var dateTimeLate = new Date(Date.UTC(2009, 4, 12, 23, 59, 59));

        dateProperty = dateVar;
        dateTimeProperty = dateTimeEarly;
        dateTimeProperty2 = dateTimeLate;

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
