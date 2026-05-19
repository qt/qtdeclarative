pragma Singleton
import QtQuick

Item {
    id: root

    property color background: "#121212"
    property color textColor: "#FEFEFE"

    QtObject {
        id: darkTheme
        objectName: "darkTheme"
        property color bg: "#121212"
        property color text: "#FEFEFE"
    }

    property alias currentTheme: darkTheme
}
