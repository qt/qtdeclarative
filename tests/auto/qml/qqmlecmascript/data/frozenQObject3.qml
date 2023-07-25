import QtQml
import test

QtObject {
    id: root

    property FrozenObjects a: FrozenObjects { objectName: "a" }
    property FrozenObjects b: FrozenObjects { objectName: "b" }

    // Create wrappers and immediately discard them
    objectName: a.getConst().objectName + "/" + b.getNonConst().objectName

    // Create a non-const wrapper and retain it
    property var objNonConst: a.getNonConst()

    // Create a const wrapper and retain it
    property var objConst: b.getConst()

    property int gcs: 0

    property Timer t: Timer {
        interval: 1
        running: true
        repeat: true
        onTriggered: {
            gc();
            ++root.gcs;
        }
    }
}
