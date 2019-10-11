import QtQuick 2.10
import QtQuick.Layouts 1.3

Item {
    id: root

    visible: true

    width: 400
    height: 200
    property bool switchToRight: false
    property alias stayingRectX: stayingRect.x

    RowLayout {
        id: topLayout

        anchors.fill: parent

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                id: leftRect

                width: parent.width*(2/3)
                height: width
                anchors.centerIn: parent

                color: "red"

                Rectangle {
                    id: stayingRect

                    x: 70; y: 70
                    width: 50; height: 50

                    color: "yellow"
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                id: rightRect

                width: parent.height*(2/3)
                height: width
                anchors.centerIn: parent

                color: "green"
                rotation: 45
            }
        }
    }

    states: State {
        name: "switchToRight"

        ParentChange {
            target: stayingRect
            parent: rightRect
            width: 70
        }

    }

    state: root.switchToRight ? "switchToRight" : ""
}
