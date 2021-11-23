import QtQuick

Item {
    property var oob: children[12]
    Component.onCompleted: console.log("oob", children[11])
}
