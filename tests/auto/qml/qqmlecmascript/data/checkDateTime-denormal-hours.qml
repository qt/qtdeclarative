import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateTimeProperty = new Date(2019, 11, 31, 1440) // 2020-2-29 0:0:0
        boolProperty = (dateTimeProperty.getFullYear() == 2020
                        && dateTimeProperty.getMonth() == 1
                        && dateTimeProperty.getDate() == 29
                        && dateTimeProperty.getHours() == 0
                        && dateTimeProperty.getMinutes() == 0
                        && dateTimeProperty.getSeconds() == 0
                        && dateTimeProperty.getMilliseconds() == 0)
    }
}
