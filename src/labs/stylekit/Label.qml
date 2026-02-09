// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.Label {
    id: control

    leftPadding: styleReader.leftPadding
    topPadding: styleReader.topPadding
    rightPadding: styleReader.rightPadding
    bottomPadding: styleReader.bottomPadding

    font: styleReader.font

    color: styleReader.text.color
    linkColor: control.palette.link

    StyleKitControl.controlType: styleReader.type
    StyleKitReader {
        id: styleReader
        type: StyleKitReader.Label
        enabled: control.enabled
        focused: control.activeFocus
        palette: control.palette
    }

    background: BackgroundDelegate {
        parentControl: control
        backgroundProperties: styleReader.background
    }
}
