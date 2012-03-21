import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateProperty = new Date("1982-11-25")
        dateTimeProperty = new Date("2009-05-12T13:22:01")
    }
}
