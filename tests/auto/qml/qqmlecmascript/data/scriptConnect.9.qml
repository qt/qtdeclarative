import Qt.test
import QtQuick

MyQmlObject {
    id: root
    property int a: 0

    signal someSignal

    function disconnectSignal() {
        root.someSignal.disconnect(other.MyQmlObject, root.test)
    }

    function destroyObj() {
        other.destroy()
    }

    function test() {
        other.MyQmlObject.value2++
        root.a = other.MyQmlObject.value2
    }

    property MyQmlObject obj
    obj: MyQmlObject {
        id: other
        MyQmlObject.value2: 0
    }

    Component.onCompleted: root.someSignal.connect(other.MyQmlObject, root.test)
}
