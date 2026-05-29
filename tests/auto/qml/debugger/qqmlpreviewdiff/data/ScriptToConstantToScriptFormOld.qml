// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Base form type mimicking CoffeeCardForm.ui.qml.
// The outOfDialog Rectangle's visible binding references root.cupsLeft.

import QtQuick

Item {
    id: root
    property int cupsLeft: 0
    property alias outOfDialog: outOfDialog
    property alias innerRect: innerRect

    Rectangle {
        id: innerRect
        width: 200
        height: 200

        Rectangle {
            id: outOfDialog
            width: 100
            height: 50
            visible: (root.cupsLeft != 0) ? false : true
        }
    }
}
