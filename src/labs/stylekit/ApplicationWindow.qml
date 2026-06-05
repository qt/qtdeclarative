// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.ApplicationWindow {
    id: control

    // Padding needs to take SafeArea into account, otherwise the contentItem will
    // be placed under for example the menu bar. The SafeArea attached object that
    // takes this into account is placed on the contentItem.parent
    // ("ApplicationWindowContentControl"), and not directly on the ApplicationWindow itself.
    topPadding: contentItem.parent.SafeArea.margins.top + styleReader.topPadding
    leftPadding: contentItem.parent.SafeArea.margins.left + styleReader.leftPadding
    rightPadding: contentItem.parent.SafeArea.margins.right + styleReader.rightPadding
    bottomPadding: contentItem.parent.SafeArea.margins.bottom + styleReader.bottomPadding

    color: styleReader.background.color

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.ApplicationWindow
        palette: control.palette
    }
}
