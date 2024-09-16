import QtQuick

Item {
    id: root

    Component {
        id: comp
        Rectangle {
            color: "blue"
        }
    }

    Component.onCompleted: comp.createObject(root, {"width": 200, "height": 200})
}
