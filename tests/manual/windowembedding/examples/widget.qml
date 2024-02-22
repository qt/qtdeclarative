// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import ".."

Rectangle {
    id: rootItem
    gradient: Gradient.TemptingAzure
    width: 500; height: 500
    visible: true

    WindowContainer {
        id: foreignWindowContainer
        window: widgetWindow
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 20
        }
        // Anchoring to bottom will result in the widget resizing during
        // the transition, which slows down things quite a bit, so as a
        // workaround, base the height of the widget on the outer window.
        height: root.height - 80
    }
}
