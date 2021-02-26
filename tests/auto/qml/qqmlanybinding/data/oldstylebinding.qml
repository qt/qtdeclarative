import QtQml

QtObject {
    id: root
    property int trigger: 1
    property int prop: trigger
    property alias a2: obj1.a1

    property QtObject obj: QtObject {
        id: obj1
        property alias a1: root.prop
    }
}
