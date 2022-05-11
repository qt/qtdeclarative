import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    height: 200

    RowLayout {
        spacing: 2
        width: 300

        DelayButton {
            text: "Delay 3000"
            delay: 3000
            progress: 0.7
        }

        DelayButton {
            text: "Delay null"
            delay: null
            progress: 0
        }

        DelayButton {
            text: "Default"
        }
    }

    RowLayout {
        spacing: 2
        width: 300

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
