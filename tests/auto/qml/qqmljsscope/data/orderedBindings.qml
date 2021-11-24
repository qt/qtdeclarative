import QtQuick
QtObject {
    property QtObject p: QtObject {}
    Behavior on p {}

    property list<QtObject> items: [
        Item {},
        Text{}
    ]
}
