import QtQuick

Item {
    id: root

    property bool bindingActive: false

    width: 640
    height: 480
    property Item background

    Binding {
        root.background: Rectangle {
            implicitWidth: 10
            implicitHeight: 10
            color: "blue"
        }

        when: root.bindingActive
    }
}
