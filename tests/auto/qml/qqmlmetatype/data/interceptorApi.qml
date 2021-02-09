import QtQml
import test

QtObject {
    id: root
    default property list<QtObject> children
    Interceptor on objectName { id: interceptor }
    property string s: "foo"
    objectName: s

    property Interceptor i: interceptor
    Component.onCompleted: () => root.objectName = "bar"
}
