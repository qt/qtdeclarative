import QtQuick 2.0
import QtQuick.Layouts 1.3
import Qt.labs.controls 1.0

Column {
    width: 300

    //! [1]
    TabBar {
        id: bar
        width: parent.width
        TabButton {
            text: qsTr("Home")
        }
        TabButton {
            text: qsTr("Discover")
        }
        TabButton {
            text: qsTr("Activity")
        }
    }

    StackLayout {
        width: parent.width
        currentIndex: bar.currentIndex
        Item {
            id: homeTab
        }
        Item {
            id: discoverTab
        }
        Item {
            id: activityTab
        }
    }
    //! [1]
}
