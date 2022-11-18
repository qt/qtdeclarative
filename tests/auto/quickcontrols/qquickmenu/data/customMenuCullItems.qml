import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 200
    height: 200
    property alias menu: menu

    Menu {
        id: menu

        contentItem: FocusScope {
            implicitHeight: view.implicitHeight
            Button {
                anchors {
                    top: parent.top
                    topMargin: 5
                    horizontalCenter: parent.horizontalCenter
                }
                z: 1
                text: "Button Up"
                visible: view.interactive
            }
            ListView {
                id: view
                width: parent.width
                implicitHeight: Math.min(contentHeight, 300)
                model: menu.contentModel

                clip: true
                currentIndex: menu.currentIndex
                ScrollIndicator.vertical: ScrollIndicator {}
            }
            Button {
                anchors {
                    bottom: parent.bottom
                    bottomMargin: 5
                    horizontalCenter: parent.horizontalCenter
                }
                z: 1
                text: "Button Down"
                visible: view.interactive
            }
        }

        Repeater {
            model: 20
            MenuItem {
                objectName: "Item: " + modelData
                text: objectName
            }
        }
    }
}
