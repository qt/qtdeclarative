import QtQml

QtObject {
    id: root
    property int x: 42
    property QtObject obj: QtObject {
        property int y: root.x
    }
}
