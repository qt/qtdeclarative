// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Window

Window {
    id: root
    visible: true
    width: 200
    height: 200

    property bool overridden: false
    property bool receivedA: false
    property bool receivedB: false
    Item {
        Keys.onShortcutOverride: (e) => e.accepted = root.overridden = (e.key === Qt.Key_A)

        Item {
            focus: true
            Shortcut {
                sequence: "A"
                onActivated: root.receivedA = true
            }
            Shortcut {
                sequence: "B"
                onActivated: root.receivedB = true
            }
        }
    }
}
