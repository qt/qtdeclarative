import QtQml 2.15

QtObject {
    id: root
    Component.onCompleted: {
        let comp = Qt.createComponent("dynamic.qml");
        let inst = comp.createObject(root, { testObj: new Set(), });
        objectName = inst.use();
    }
}
