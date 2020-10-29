import Qt.test 1.0
import QtQml 2.15


MyTypeObject {
    Component.onCompleted: {
        dateTimeProperty = new Date("1991-08-25 20:57:08 GMT+0000")
        boolProperty = !Number.isNaN(dateTimeProperty)
    }
}
