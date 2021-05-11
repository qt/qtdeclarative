import QtQuick

Item {
    // Inline component must be two components deep to hit an assert
    QtObject {
        id: oneDown
        component OneDepth : QtObject {
            // Triggered by any script binding
            objectName: "hel" + "lo"
        }
    }
}
