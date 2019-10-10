import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateProperty = new Date(2019, 1, 29) // March 1st
        boolProperty = (dateProperty.getFullYear() == 2019
                        && dateProperty.getMonth() == 2
                        && dateProperty.getDate() == 1)
    }
}
