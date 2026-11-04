import QtQuick
import Qt.labs.StyleKit

Style {
    // Force light theme to be used, so that the test is not dependent on the system theme
    themeName: "Light"

    control {
        background.leftMargin: 1 // final assignment
        leftPadding: 1
        rightPadding: 1
        topPadding: 1
        bottomPadding: 1
        spacing: 1
    }

    abstractButton {
        leftPadding: 2 // final assignment
        rightPadding: 2
        topPadding: 2
        bottomPadding: 2
        spacing: 2
    }

    button {
        rightPadding: 3 // final assignment
        topPadding: 3
        bottomPadding: 3
        spacing: 3
    }

    light: Theme {
        control {
            topPadding: 4 // final assignment
            bottomPadding: 4
            spacing: 4
        }

        abstractButton {
            bottomPadding: 5 // final assignment
            spacing: 5
        }

        button {
            spacing: 6 // final assignment
        }
    }
}