import QtQuick
import QtQuick.Window
import QtQuick.Controls.Basic

ApplicationWindow {
    id: root
    width: 200
    height: 200
    color: "#222"

    Timer {
        running: true
        interval: 100
        repeat: true

        function randomInteger(min, max) {
            return Math.floor(Math.random() * (max - min + 1)) + min;
        }

        onTriggered: {
            for (let i = 0; i < 10; ++i) {
                listModel.append({
                    pointObjectX: randomInteger(0, 40),
                    pointObjectY: randomInteger(0, 40)
                })
                listModel.remove(randomInteger(0, listModel.count - 1), 1)
            }
        }
    }

    Repeater {
        model: ListModel {
            id: listModel

            ListElement {
                pointObjectX: 101
                pointObjectY: 100
            }
            ListElement {
                pointObjectX: 200
                pointObjectY: 201
            }
            ListElement {
                pointObjectX: 301
                pointObjectY: 300
            }
        }

        delegate: Rectangle {
            id: delegateRoot
            x: pointObjectX
            y: pointObjectY
            width: 40
            height: 40
            color: Qt.rgba(Math.random(), Math.random(), Math.random(), 1)
            border.color: "white"
            border.width: 1

            required property int pointObjectX
            required property int pointObjectY

            Loader {
                active: delegateRoot.x % 2 == 1 || delegateRoot.y % 2 == 1
                asynchronous: true
                anchors.fill: parent
                sourceComponent: Popup {
                    width: 40
                    height: 40
                    visible: true
                }
            }
        }
    }
}
