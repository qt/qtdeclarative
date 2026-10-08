// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias contextMenu: contextMenu

    Menu {
        id: contextMenu
        objectName: "menu"
        popupType: Popup.Native
        // For debugging in case of warnings in QPA.
        title: objectName

        Action {
            objectName: text
            text: "action1"
            shortcut: "A"
        }

        MenuItem {
            objectName: text
            action: Action {
                text: "menuItemAction"
                objectName: text
                shortcut: "B"
            }
        }

        Menu {
            id: subMenu
            title: "subMenu"
            objectName: title
            popupType: Popup.Native
            // TODO: remove me when the defaults are true

            Action {
                objectName: text
                text: "subAction1"
                shortcut: "1"
            }
        }
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: contextMenu.popup()
    }
}
