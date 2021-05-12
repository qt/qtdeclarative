import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateProperty = new Date(2019, 2, 0); // Feb 28th
        boolProperty = (dateProperty.getUTCFullYear() == 2019
                        && dateProperty.getUTCMonth() == 1
                        && dateProperty.getUTCDate() == 28);
    }
}
