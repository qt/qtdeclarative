// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes
import QtQuick.Controls
import QtQuick.Dialogs

ControlledShape {
    fillRule: ShapePath.OddEvenFill
    delegate: [
        PathText {
          text: "foobar"
          font: fontDialog.selectedFont
        }
    ]

    FontDialog {
        id: fontDialog
        currentFont.family: "Graziano"
        currentFont.pixelSize: 500
    }

    Button {
        anchors.top: parent.bottom
        anchors.left: parent.left
        text: "Select font"
        onClicked: fontDialog.open()
    }

}
