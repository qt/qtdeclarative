import QtQuick
import QtQuick.Window
import QtQuick.Controls

Item {
    Component.onCompleted: control.focus = true
    width: 640
    height: 480

    Column {
        anchors.top: parent.top
        anchors.topMargin: 10
        spacing: 10

        Control {
            id: control
            implicitWidth: 100
            implicitHeight: 20
            objectName: "control"
            activeFocusOnTab: true

            Menu {
                id: contextMenu
                objectName: "contextMenu"
                MenuItem {
                    text: "Hello"
                }
                MenuItem {
                    text: "World"
                }
            }
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function onClicked(mouseEvent) {
                    if (mouseEvent.button == Qt.RightButton)
                        contextMenu.visible = true
                }
            }
        }

        ComboBox {
            id: combobox
            objectName: "combobox"
            model: ["Banana", "Apple", "Coconut"]
            activeFocusOnTab: true
        }

        ComboBox {
            id: editcombo
            objectName: "editcombo"
            editable: true
            model: ["Kiwi", "Mango", "Pomelo"]
            activeFocusOnTab: true
        }

        SpinBox {
            id: spinbox
            objectName: "spinbox"
            from: 0
            to: 100
            value: 20
            editable: true
            activeFocusOnTab: true
        }

        Control {
            id: customText
            objectName: "customText"
            implicitWidth: 100
            implicitHeight: 50
            contentItem: TextInput {
                text: parent.visualFocus ? "focus" : "no focus"
            }
            activeFocusOnTab: true
        }

        Control {
            id: customItem
            objectName: "customItem"
            implicitWidth: 100
            implicitHeight: 50
            contentItem: Rectangle {
                anchors.fill: parent
                color: parent.activeFocus ? "red" : "blue"
                opacity: 0.3
            }
            focusPolicy: Qt.WheelFocus
            activeFocusOnTab: true
        }

        TextField {
            id: textfield
            objectName: "textfield"
            text: "test123"
            activeFocusOnTab: true
        }
    }
}
