pragma ComponentBehavior: Bound

import QtQml
import QtQml.Models

QtObject {
    id: root
    objectName: "root"
    property Instantiator undefinedModelData: Instantiator {
        model: [1, 2, 3]
        delegate: QtObject {
            property var notHere: modelData
            objectName: root.objectName + notHere
        }
    }

    property Instantiator requiredModelData: Instantiator {
        model: [1, 2, 3]
        delegate: QtObject {
            required property int modelData
            objectName: root.objectName + modelData
        }
    }
}
