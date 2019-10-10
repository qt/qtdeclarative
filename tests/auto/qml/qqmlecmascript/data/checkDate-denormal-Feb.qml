import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateProperty = new Date(2019, 2, 0) // Feb 28th
        boolProperty = (dateProperty.getFullYear() == 2019
                        && dateProperty.getMonth() == 1
                        && dateProperty.getDate() == 28)
    }
}
