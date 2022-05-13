// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 200
    height: 200
    visible: true

    signal focusScopeKeyPressed
    signal focusPopupKeyPressed

    property alias fileMenu: fileMenu
    property alias toolButton: toolButton
    property alias focusScope: focusScope
    property alias focusPopup: focusPopup

    header: ToolBar {
        ToolButton {
            id: toolButton
            text: qsTr("File")
            onClicked: fileMenu.open()
            focusPolicy: Qt.TabFocus

            Menu {
                id: fileMenu
                y: parent.height

                MenuItem {
                    text: qsTr("New")
                }
                MenuItem {
                    text: qsTr("Open")
                }
                MenuItem {
                    text: qsTr("Close")
                }
            }
        }
    }

    FocusScope {
        id: focusScope
        focus: true
        anchors.fill: parent

        Keys.onSpacePressed: focusScopeKeyPressed()
    }

    Popup {
        id: focusPopup
        focus: true
        width: parent.width
        height: parent.height

        Item {
            focus: true
            Keys.onSpacePressed: focusPopupKeyPressed()
        }
    }
}

