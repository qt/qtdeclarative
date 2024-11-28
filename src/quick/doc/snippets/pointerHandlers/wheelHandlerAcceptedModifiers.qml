// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![entire]
import QtQuick

Rectangle {
    width: 170; height: 120
    color: "green"; antialiasing: true

    WheelHandler {
        property: "rotation"
        acceptedModifiers: Qt.ControlModifier
    }

    WheelHandler {
        property: "scale"
        acceptedModifiers: Qt.NoModifier
    }
}
//![entire]
