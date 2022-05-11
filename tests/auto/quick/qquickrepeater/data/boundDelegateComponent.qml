pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root
    objectName: "root"
    Repeater {
        id: undefinedModelData
        model: ["aa", "bb", "cc"]
        Item {
            property var notHere: modelData
            objectName: root.objectName + notHere
        }
    }

    Repeater {
        id: requiredModelData
        model: ["aa", "bb", "cc"]
        Item {
            required property string modelData
            objectName: root.objectName + modelData
        }
    }
}
