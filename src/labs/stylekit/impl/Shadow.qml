// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Effects
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

RectangularShadow {
    required property QtObject control
    required property StyleKitDelegateProperties delegateProperties

    width: parent.width
    height: parent.height
    color: delegateProperties.shadow.color
    opacity: delegateProperties.shadow.opacity
    blur: delegateProperties.shadow.blur
    scale: delegateProperties.shadow.scale
    radius: delegateProperties.radius
    topLeftRadius: delegateProperties.topLeftRadius
    topRightRadius: delegateProperties.topRightRadius
    bottomLeftRadius: delegateProperties.bottomLeftRadius
    bottomRightRadius: delegateProperties.bottomRightRadius
    visible: delegateProperties.shadow.visible
    offset: Qt.vector2d(
                delegateProperties.shadow.horizontalOffset,
                delegateProperties.shadow.verticalOffset)
}
