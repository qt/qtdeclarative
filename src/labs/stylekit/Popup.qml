// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.Popup {
    id: control

    StyleKitControl.controlType: __styleReader.type
    readonly property StyleKitReader __styleReader: StyleKitReader {
        // TODO: making StyleKitReader a child object of T.Popup makes the
        // popup not open on press. So use a __styleReader property for now
        // until we know the reason why.
        type: StyleKitReader.Popup
        enabled: control.enabled
        focused: control.activeFocus
        palette: control.palette
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    leftPadding: __styleReader.leftPadding
    topPadding: __styleReader.topPadding
    rightPadding: __styleReader.rightPadding
    bottomPadding: __styleReader.bottomPadding
    font: __styleReader.font

    background: BackgroundDelegate {
        quickControl: control
        backgroundProperties: control.__styleReader.background
    }
}
