import QtQuick

Item {
    Component.onCompleted: Qt.callLater(console.log, "hello, World")
}
