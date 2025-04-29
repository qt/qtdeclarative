// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias rootItem: rootItem
    property alias popup: popup
    property alias button: button

    Item {
        id: rootItem
        focus: true
        Component.onCompleted: forceActiveFocus()
    }

    Popup {
        id: popup
        focus: true
        z: 1
        popupType: Popup.Item
        contentItem: Item {
            Button {
                id: button
                onVisibleChanged: {
                    if (visible)
                        forceActiveFocus()
                }
            }
        }
    }
}
