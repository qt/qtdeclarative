// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

ControlledShape {
    strokeColor: "black"
    strokeWidth: 1
    fillColor: "black"

    delegate: [
        PathText {
            x: 0
            y: 0
            text: qsTr("Qt!")
            font.family: "Arial"
            font.pixelSize: 150
        }
    ]
}
