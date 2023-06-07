import QtQuick

Rectangle {
    property string hello

    id: root

    Connections {
        target: root
        function onHelloChanged(argument) {}
    }
}
