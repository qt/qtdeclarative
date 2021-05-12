import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        // QTBUG-78996
        dateProperty = new Date(2019, 9, 3)
        boolProperty = (dateProperty.getUTCFullYear() == 2019
                        && dateProperty.getUTCMonth() == 9
                        && dateProperty.getUTCDate() == 3);
    }
}
