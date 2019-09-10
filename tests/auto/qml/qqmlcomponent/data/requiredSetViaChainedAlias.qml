import QtQml 2.12

QtObject {
    id: stuff
    required property int x;
    property alias y: stuff.x
    property alias z: stuff.y
    z: 5
}
