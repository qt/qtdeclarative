pragma Singleton
import QtQuick

Item {
    id: root

    property color background: "#ffffff"
    property color textColor: "#121111"

    QtObject {
        id: darkTheme
        objectName: "darkTheme"
        property color bg: "#ffffff"
        property color text: "#121111"
    }

    property alias currentTheme: darkTheme
}
