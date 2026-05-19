pragma Singleton
import QtQuick

Item {
    id: root

    component Theme : QtObject {
        property color background
        property color textColor
    }

    Theme {
        id: darkTheme
        background: "#121212"
        textColor: "#FEFEFE"
    }

    Theme {
        id: lightTheme
        background: "#ffffff"
        textColor: "#121111"
    }

    property Theme currentTheme: darkTheme
    property alias dark: darkTheme
    property alias light: lightTheme
}
