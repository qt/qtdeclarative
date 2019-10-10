import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        // QTBUG-78996
        dateProperty = new Date(2019, 9, 3)
        boolProperty = (dateProperty.getFullYear() == 2019
                        && dateProperty.getMonth() == 9
                        && dateProperty.getDate() == 3)
    }
}
