import Qt.test 1.0
import QtQuick 2.0

MyTypeObject {
    Component.onCompleted: {
        dateProperty = new Date(2019, 1, 29); // March 1st
        boolProperty = (dateProperty.getUTCFullYear() == 2019
                        && dateProperty.getUTCMonth() == 2
                        && dateProperty.getUTCDate() == 1);
    }
}
