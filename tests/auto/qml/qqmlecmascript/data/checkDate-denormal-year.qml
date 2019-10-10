import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateProperty = new Date(2019, 12, 0) // Dec 31
        boolProperty = (dateProperty.getFullYear() == 2019
                        && dateProperty.getMonth() == 11
                        && dateProperty.getDate() == 31)
    }
}
