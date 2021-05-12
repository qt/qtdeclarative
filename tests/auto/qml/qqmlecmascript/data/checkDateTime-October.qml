import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateTimeProperty = new Date(2019, 9, 3, 12);
        boolProperty = (dateTimeProperty.getFullYear() == 2019
                        && dateTimeProperty.getMonth() == 9
                        && dateTimeProperty.getDate() == 3
                        && dateTimeProperty.getHours() == 12
                        && dateTimeProperty.getMinutes() == 0
                        && dateTimeProperty.getSeconds() == 0
                        && dateTimeProperty.getMilliseconds() == 0
                        // QTBUG-78996: day of the week:
                        && dateTimeProperty.getDay() == 4);
    }
}
