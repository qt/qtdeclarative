import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Controls.Basic

ApplicationWindow {
    id: appWindow

    // QQuickDeferredPointer<QQuickItem> background;
    background: Rectangle {
        id: backgroundRect
        color: "black"
    }

    // internal property handle
    ColorDialog {
        id: colorDialog
        options: ColorDialog.DontUseNativeDialog
    }

    // internal property upButton
    // internal property textField
    FolderDialog {
        id: folderDialog
        options: FolderDialog.DontUseNativeDialog
    }

    // indicator property of AbstractButton
    Button {
        id: basicButton
        indicator: Rectangle {
            id: basicButtonIndicator
            color: "pink"
        }
    }

    Dial {
        id: dial
        // QQuickDeferredPointer<QQuickItem> handle;
        handle: Item {
            Rectangle {
                id: dialRect
            }
        }
    }

    GroupBox {
        id: groupBox
        // QQuickDeferredPointer<QQuickItem> label;
        label: Label {
            id: groupBoxLabel
            text: "yo"
        }
    }

    Label {
        id: label
        text: "yo2"
        background: Rectangle {
            id: labelBackground
            color: "green"
        }
    }

    Menu {
        id: menu

        MenuItem {
            id: menuItem
            text: "New..."
            arrow: Rectangle {
                id: menuItemArrow
                color: "pink"
            }
        }
    }

    ScrollBar {
        id: scrollbar
    }

    SpinBox {
        id: spinBox

        up.indicator: Rectangle {
            id: spinBoxUpIndicator
            color: "pink"
        }
        down.indicator: Rectangle {
            id: spinBoxDownIndicator
            color: "blue"
        }
    }

    Control {
        id: genericControl
        background: Rectangle {
            id: genericControlBackground
            color: "red"
        }
        contentItem: Canvas {
            id: genericControlContentItem
        }
    }

    ComboBox {
        id: comboBox
        model: ["foo", "bar"]

        popup: Popup {
            id: comboBoxPopup
            contentItem: ListView {
                ScrollBar.vertical: ScrollBar {}
            }
        }
        indicator: Item {
            id: emptyPopupItem
        }
    }

    Slider {
        id: slider
        // QQuickDeferredPointer<QQuickItem> handle;
        handle: Item {
            Rectangle {
                id: rect
            }
        }
    }

    RangeSlider {
        id: rangeSlider
        first.handle: Item {
            Rectangle {
                id: rangeSliderRect1
            }
        }
        second.handle: Item {
            Rectangle {
                id: rangeSliderRect2
            }
        }
    }

    TextArea {
        id: textArea
        background: Rectangle {
            id: textAreaBackground
            color: "pink"
        }
    }

    TextField {
        id: textField
        background: Rectangle {
            id: textFieldBackground
            color: "pink"
        }
    }

    TabBar {
        id: bar
        TabButton {
            text: qsTr("One")
        }
        TabButton {
            text: qsTr("Two")
        }
    }
}

