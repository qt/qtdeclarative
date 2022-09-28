import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    height: 200

    RowLayout {
        Layout.margins: 20
        spacing: 3

        DelayButton {
            text: "Delay 3000"
            delay: 3000
            progress: 0.7
        }

        DelayButton {
            text: "Delay null"
            delay: 0
            progress: 0
        }

        DelayButton {
            text: "Default"
        }
    }

    RowLayout {
        Layout.margins: 20
        spacing: 2

        DelayButton {
            text: "Delay 0"
            delay: 0
            progress: 0.1
        }

        DelayButton {
            enabled: false
            text: "Delay 500"
            delay: 500
            progress: 1.0
        }
        DelayButton {
            text: "Delay 1000"
            delay: 1000
            down: true
            progress: 0.0
        }
    }
}
