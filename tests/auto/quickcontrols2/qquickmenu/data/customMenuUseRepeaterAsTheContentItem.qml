import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 200
    height: 200
    property alias menu: menu

    Menu {
        id: menu
        visible: true

        contentItem: FocusScope {
            implicitHeight: flickable.height

            Button {
                anchors {
                    top: parent.top
                    topMargin: 5
                    horizontalCenter: parent.horizontalCenter
                }
                z: 1
                text: "Button Up"
            }

            Flickable {
                id: flickable
                width: parent.width
                height: Math.min(contentHeight, 300)
                contentHeight: repeaterLayout.implicitHeight
                clip: true

                ScrollIndicator.vertical: ScrollIndicator {}

                ColumnLayout {
                    id: repeaterLayout
                    width: parent.width

                    Repeater {
                        model: menu.contentModel
                    }
                }
            }

            Button {
                anchors {
                    bottom: parent.bottom
                    bottomMargin: 5
                    horizontalCenter: parent.horizontalCenter
                }
                z: 1
                text: "Button Down"
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
