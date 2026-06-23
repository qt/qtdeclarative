// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Window {
    width: 400
    height: 400

    property alias popup: popup
    property alias draggableRect: draggableRect

    Popup {
        id: popup
        popupType: Popup.Window
        x: 50
        y: 100
        width: 200
        height: 200

        Rectangle {
            id: draggableRect
            width: 50
            height: 50
            color: "steelblue"

            DragHandler {}
        }
    }
}
