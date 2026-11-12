import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 400
    height: 400

    property point pressPos
    property bool showItToo: true

    Button {
        id: button
        objectName: "tomato"
        text: "tomato"
        x: 100
        y: 100

        ContextMenu.onRequested: position => {
            root.pressPos = position
            const menu = buttonMenu.createObject(button)
            if (root.showItToo)
                menu.popup(position)
            else
                ContextMenu.menu = menu
        }
    }

    Component {
        id: buttonMenu
        Menu {
            MenuItem {
                text: qsTr("Eat tomato")
            }
            MenuItem {
                text: qsTr("Throw tomato")
            }
            MenuItem {
                text: qsTr("Squash tomato")
            }
        }
    }
}
