import QtQml 2.14

QtObject {
    id: root
    property QtObject o1: QtObject {
        property alias a: root
        property alias a: root
    }
}
