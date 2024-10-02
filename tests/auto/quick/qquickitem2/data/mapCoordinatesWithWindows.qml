import QtQuick

QtObject {
    id: root

    property Window windowA: Window {
        visible: true
        flags: Qt.FramelessWindowHint
        color: "green"
        x: 100; y: 100
        width: 300; height: 300

        Rectangle {
            objectName: "childItem"
            x: 50; y: 50
            width: 100; height: 100
            color: "red"
        }

        WindowContainer {
            x: 100; y: 100
            width: 100; height: 100
            window: Window {
                objectName: "childWindow"
                color: "blue"

                Rectangle {
                    objectName: "childItemInChildWindow"
                    x: 30; y: 30
                    width: 50; height: 50
                    color: "orange"
                }
            }
        }
    }

    property Window windowB: Window {
        visible: true
        flags: Qt.FramelessWindowHint
        color: "magenta"
        x: 500; y: 200
        width: 200; height: 200

        Rectangle {
            objectName: "childItem"
            x: 50; y: 50
            width: 100; height: 100
            color: "cyan"
        }
    }

    property Item itemWithoutWindowA: Item {
        x: 20; y: 20
    }
    property Item itemWithoutWindowB: Item {
        x: 40; y: 40
        Item {
            objectName: "childItemWithoutWindow"
            x: 30; y: 30
        }
    }
}
