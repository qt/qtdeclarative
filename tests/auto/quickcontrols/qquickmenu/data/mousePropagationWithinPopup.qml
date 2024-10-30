import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 360
    height: 360

    property alias popup: popup
    property alias nestedMenu: nestedMenu
    property alias mouseArea: mouseArea

    Popup {
        id: popup
        width: 200
        height: 200
        MouseArea {
            id: mouseArea
            anchors.fill: parent
        }
        Menu {
            id: nestedMenu
            width: 100
            height: 100
            MenuItem {
                text: "Menu item 1"
            }
            MenuItem {
                text: "Menu item 2"
                enabled: false
            }
        }
    }
}
