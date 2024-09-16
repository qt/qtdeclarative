// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
                popupType: Popup.Item

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
        popupType: Popup.Item
        width: parent.width
        height: parent.height

        Item {
            focus: true
            Keys.onSpacePressed: focusPopupKeyPressed()
        }
    }
}

