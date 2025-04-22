// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.Frame {
    id: control

    implicitWidth: Math.max((background.minimumWidth || implicitBackgroundWidth)
                            + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max((background.minimumHeight || implicitBackgroundHeight)
                            + topInset + bottomInset,
                            contentHeight + topPadding + bottomPadding)

    topPadding: __config.topPadding || 0
    bottomPadding: __config.bottomPadding || 0
    leftPadding: __config.leftPadding || 0
    rightPadding: __config.rightPadding || 0

    topInset: -__config.topInset || 0
    bottomInset: -__config.bottomInset || 0
    leftInset: -__config.leftInset || 0
    rightInset: -__config.rightInset || 0

    readonly property string __currentState: !control.enabled ? "disabled" : "normal";
    readonly property var __config: Config.controls.frame[__currentState] || {}

    background: StyleImage {
        imageConfig: control.__config.background
    }
}
