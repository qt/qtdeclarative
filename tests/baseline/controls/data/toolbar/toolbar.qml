import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {

    height: 200

    header: ToolBar {
        Label {
            text: qsTr("Toolbar 1")
        }
    }

    ToolBar {
        RowLayout {
            LayoutMirroring.enabled: true

            ToolButton {
                text: "Button"
            }

            ToolSeparator {
            }

            ToolButton {
                text: "Button"
                enabled: false
            }

            ToolSeparator {
                enabled: false
            }

            ToolButton {
                text: "Button"
                focus: true
            }

            ToolSeparator {
                orientation: Qt.Horizontal
            }

            ToolButton {
                text: "Button"
                flat: true
            }

            ToolSeparator {
                LayoutMirroring.enabled: true
            }

            ToolButton {
                text: "Button"
                highlighted: true
            }

            ToolButton {
                text: "Button"
                down: true
            }

            ToolButton {
                text: "Button"
                checked: true
            }

            ToolButton {
                text: "Button"
                checkable: true
            }

            ToolButton {
                text: "Button"
                LayoutMirroring.enabled: true
            }
        }
    }

    footer: ToolBar {
        enabled: false

        Label {
            text: qsTr("Toolbar 2")
        }
    }
}
