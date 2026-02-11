// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Effects
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

RectangularShadow {
    required property QtObject control
    required property DelegateStyle delegateStyle

    width: parent.width
    height: parent.height
    color: delegateStyle.shadow.color
    opacity: delegateStyle.shadow.opacity
    blur: delegateStyle.shadow.blur
    scale: delegateStyle.shadow.scale
    radius: delegateStyle.radius
    topLeftRadius: delegateStyle.topLeftRadius
    topRightRadius: delegateStyle.topRightRadius
    bottomLeftRadius: delegateStyle.bottomLeftRadius
    bottomRightRadius: delegateStyle.bottomRightRadius
    visible: delegateStyle.shadow.visible
    offset: Qt.vector2d(
                delegateStyle.shadow.horizontalOffset,
                delegateStyle.shadow.verticalOffset)
}
