import Test 1.0
import Test 1.0 as Namespace
import QtQuick 2.0

QtObject {
    id: root
    MyQmlObject.value: 10
    Namespace.MyQmlObject.value2: 13
    property string nameCopy
    property int lengthCopy

    Component.onCompleted: {
        root.nameCopy = root.MyQmlObject.name
        root.lengthCopy = root.MyQmlObject.length
        root.MyQmlObject.name = "modified"
        root.MyQmlObject.length = -42
    }
}
