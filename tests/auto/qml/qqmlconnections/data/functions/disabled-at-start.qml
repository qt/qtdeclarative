import QtQuick 2.9

Item {
    id: root

    property bool tested: false
    signal testMe()

    Connections {
        target: root
        enabled: false
        function onTestMe() { root.tested = true; }
    }
}
