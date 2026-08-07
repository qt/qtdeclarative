import QtQuick

Item {
    width: 300
    height: 200

    WindowContainer {
        objectName: "container"
        x: 50
        y: 20
        width: 200
        height: 150

        window: Window {
            objectName: "hostedWindow"

            Rectangle {
                anchors.fill: parent
                color: "cyan"

                Accessible.role: Accessible.Button
                Accessible.name: "hosted"
            }
        }
    }

    WindowContainer {
        objectName: "emptyContainer"
        x: 50
        y: 20
        width: 200
        height: 150
    }
}
