import QtQml 2.2
QtObject {
    id: root
    property alias a: root.b
    property alias b: root.a
}
