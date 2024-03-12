// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

Rectangle {
    width: 320
    height: 240
    color: rightTap.pressed ? "tomato" : "beige"
    TapHandler {
        id: rightTap
        objectName: "right button TapHandler"
        longPressThreshold: 0.5
        acceptedButtons: Qt.RightButton
    }
}
