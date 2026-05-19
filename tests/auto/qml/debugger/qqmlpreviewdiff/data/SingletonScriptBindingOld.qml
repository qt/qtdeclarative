pragma Singleton
import QtQuick

Item {
    id: root

    property color background: "#121212"
    property color textColor: "#FEFEFE"
    property color derivedColor: Qt.lighter(background, 1.5)
}
