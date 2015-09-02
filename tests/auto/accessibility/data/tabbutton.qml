import QtQuick 2.5
import QtQuick.Window 2.2
import QtQuick.Controls 2.0

Window {
    visible: true

    TabBar {
        id: tabbar
        objectName: "TabBar"
        TabButton {
            id: tabbutton
            objectName: "tabbutton"
            text: "TabButton"
        }
    }
}
