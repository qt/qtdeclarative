import QtQml

QtObject {
    property var myItem: []
    Component.onCompleted: myItem[0] = 10
}
