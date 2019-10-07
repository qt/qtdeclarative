import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateTimeProperty = new Date(2020, 2, 1, 0, 0, 0, -1) // 2020-2-29 23:59:59.999
        boolProperty = (dateTimeProperty.getFullYear() == 2020
                        && dateTimeProperty.getMonth() == 1
                        && dateTimeProperty.getDate() == 29
                        && dateTimeProperty.getHours() == 23
                        && dateTimeProperty.getMinutes() == 59
                        && dateTimeProperty.getSeconds() == 59
                        && dateTimeProperty.getMilliseconds() == 999)
    }
}
