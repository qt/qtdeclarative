import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateProperty = new Date(2020, 2, 0); // Feb 29th
        boolProperty = (dateProperty.getUTCFullYear() == 2020
                        && dateProperty.getUTCMonth() == 1
                        && dateProperty.getUTCDate() == 29);
    }
}
