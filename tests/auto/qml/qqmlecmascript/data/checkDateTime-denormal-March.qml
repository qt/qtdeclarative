import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateTimeProperty = new Date(2019, 1, 28, 23, 59, 59, 1001) // 2019-3-1 0:0:0.001
        boolProperty = (dateTimeProperty.getFullYear() == 2019
                        && dateTimeProperty.getMonth() == 2
                        && dateTimeProperty.getDate() == 1
                        && dateTimeProperty.getHours() == 0
                        && dateTimeProperty.getMinutes() == 0
                        && dateTimeProperty.getSeconds() == 0
                        && dateTimeProperty.getMilliseconds() == 1)
    }
}
