import QtQuick
import QtQuick.Templates

Item {
    property int v: 0

    MenuBar {
        id: menuBar
        Menu {}
    }

    Instantiator {
        id: instantiator

        delegate: Menu {}

        onObjectAdded: (index, object) => {
            menuBar.menus.push(object)
        }

        onObjectRemoved: (index, object) => {
            menuBar.menus.length = 0;
        }
    }

    Timer {
        running: v < 2
        interval: 1
        repeat: true

        onTriggered: {
            instantiator.model = (++v % 2) ? 0 : 2
        }
    }
}
