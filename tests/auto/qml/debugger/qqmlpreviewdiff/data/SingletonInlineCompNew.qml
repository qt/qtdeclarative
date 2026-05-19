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
        background: "#1a1a1a"
        textColor: "#E0E0E0"
    }

    Theme {
        id: lightTheme
        background: "#fafafa"
        textColor: "#222222"
    }

    property Theme currentTheme: darkTheme
    property alias dark: darkTheme
    property alias light: lightTheme
}
