import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 600
    height: 400

    Column {
        Rectangle {
            objectName: "withMenu"
            width: 100
            height: 100
            color: "tomato"

            Accessible.role: Accessible.Button
            Accessible.name: "With menu"

            ContextMenu.menu: Menu {
                MenuItem {
                    text: qsTr("Eat tomato")
                }
            }
        }

        Rectangle {
            objectName: "withoutMenu"
            width: 100
            height: 100
            color: "steelblue"

            Accessible.role: Accessible.Button
            Accessible.name: "Without menu"
        }
    }
}
