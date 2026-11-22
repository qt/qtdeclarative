import QtQuick
import QtQuick.Controls

Item {
    width:100
    height:100

    property alias loader: menuLoader
    property bool activateLoader: false

    onActivateLoaderChanged: {
        if (activateLoader) {
            menuTimer.start()
            menuLoader.active = true
        }
    }

    Timer {
        id: menuTimer
        interval: 20
        onTriggered: menuLoader.active = false
    }

    Loader {
        id: menuLoader
        active: false
        asynchronous: true
        sourceComponent: Menu {
            contentItem: ListView {}
            Menu {}
            MenuSeparator {}
            Action {}
            Menu {
                Action {}
                Action {}
                Menu {}
            }
            MenuItem {}
            Item {
                Menu {}
            }
            Menu {
                MenuItem {}
                Action {}
                MenuItem {}
            }
            // Dummy menu items
            Menu {}
            Menu {}
            MenuSeparator {}
            Menu {}
            Menu {}
            MenuSeparator {}
            Menu {}
            Menu {}
            MenuSeparator {}
            Menu {}
            Menu {}
        }
    }
}

