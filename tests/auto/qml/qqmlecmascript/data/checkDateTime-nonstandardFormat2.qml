import Qt.test 1.0
import QtQml 2.15


MyTypeObject {
    Component.onCompleted: {
        dateTimeProperty = new Date("Sun, 25 Mar 2018 11:10:49 GMT")
        boolProperty = !Number.isNaN(dateTimeProperty)
    }
}
