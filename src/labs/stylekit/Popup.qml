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

    StyleVariation.controlType: __styleReader.controlType
    readonly property StyleReader __styleReader: StyleReader {
        // TODO: making StyleReader a child object of T.Popup makes the
        // popup not open on press. So use a __styleReader property for now
        // until we know the reason why.
        controlType: StyleReader.Popup
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

    leftInset: __styleReader.background.leftMargin
    topInset: __styleReader.background.topMargin
    rightInset: __styleReader.background.rightMargin
    bottomInset: __styleReader.background.bottomMargin

    font: __styleReader.font

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: control.__styleReader.background
    }
}
