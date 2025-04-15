// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 400
    height: 400

    property int triggeredCount

    Menu {
        objectName: "topLevelMenu"

        Menu {
            objectName: "subMenu"

            Menu {
                objectName: "subSubMenu"

                Action {
                    shortcut: StandardKey.Copy
                    onTriggered: ++root.triggeredCount
                }
            }
        }
    }
}
