import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateTimeProperty = new Date(2020, 1, 28, 23, 0, 3600) // 2020-2-29 0:0:0
        boolProperty = (dateTimeProperty.getFullYear() == 2020
                        && dateTimeProperty.getMonth() == 1
                        && dateTimeProperty.getDate() == 29
                        && dateTimeProperty.getHours() == 0
                        && dateTimeProperty.getMinutes() == 0
                        && dateTimeProperty.getSeconds() == 0
                        && dateTimeProperty.getMilliseconds() == 0)
    }
}
