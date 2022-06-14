import QtQuick
import QtQuick.Controls

Page {
    height: 200

    footer: TabBar {
        Repeater {
            model: ["First", "Second", "Third"]

            TabButton {
                text: modelData
            }
        }
    }

    header: TabBar {
        Repeater {
            model: ["First", "Second", "Third"]

            TabButton {
                text: modelData
            }
        }
    }

    TabBar {
        enabled: false
        Repeater {
            model: ["First", "Second", "Third"]

            TabButton {
                text: modelData
            }
        }
    }
}
