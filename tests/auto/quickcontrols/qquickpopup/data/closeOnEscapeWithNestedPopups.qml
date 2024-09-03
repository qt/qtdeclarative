// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    header: ToolBar {
        ToolButton {
            objectName: "optionsToolButton"
            text: "Options"
            onClicked: optionsMenu.open()

            Menu {
                id: optionsMenu
                objectName: "optionsMenu"
                x: parent.width - width
                transformOrigin: Menu.TopRight
                popupType: Popup.Item

                MenuItem {
                    objectName: "settingsMenuItem"
                    text: "Settings"
                    onTriggered: settingsDialog.open()
                }
            }
        }
    }

    Shortcut {
        sequence: "Esc"
        enabled: stackView.depth > 1
        onActivated: stackView.pop()
    }

    Component {
        id: itemComponent

        Item {}
    }

    StackView {
        id: stackView
        objectName: "stackView"
        anchors.fill: parent
        initialItem: Item {
            objectName: "initialStackViewItem"
        }

        Component.onCompleted: push(itemComponent)
    }

    Dialog {
        id: settingsDialog
        objectName: "settingsDialog"
        modal: true

        contentItem: ComboBox {
            objectName: "comboBox"
            model: 10
        }
    }
}
