import test
import QtQml

QtObject {
    id: root
    property ConstPointer constPointer: ConstPointer {}
    property bool invokableOk: false
    property bool propertyOk: false

    Component.onCompleted: () => {
        root.invokableOk = root.constPointer.test(root.constPointer)
        let device = root.constPointer.device
        try {
            device.objectName = "thawed"
        } catch (e) {
            root.propertyOk = device === root.constPointer
        }
    }
}
