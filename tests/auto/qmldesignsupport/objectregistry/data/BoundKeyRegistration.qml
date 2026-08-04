import QtQml
import QtQml.DesignSupport

QtObject {
    id: root

    property string dynamicKey: "BoundKey"

    property QtObject boundTarget: QtObject {
        property int testProp: 33
    }

    property ObjectRegistry reg: ObjectRegistry {
        target: root.boundTarget
        key: root.dynamicKey
    }
}
