// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Shapes
import Qt.labs.StyleKit

Shape {
    id: root

    required property DelegateStyle delegateStyle
    required property QtObject control

    implicitWidth: delegateStyle.width
    implicitHeight: delegateStyle.height
    width: parent.width
    height: parent.height

    preferredRendererType: Shape.CurveRenderer

    ShapePath {
        strokeWidth: root.delegateStyle.width
        strokeColor: root.delegateStyle.color
        fillColor: "transparent"
        capStyle: ShapePath.RoundCap

        PathAngleArc {
            centerX: root.width / 2
            centerY: root.height / 2
            radiusX: root.width / 2 - root.delegateStyle.width / 2
            radiusY: root.height / 2 - root.delegateStyle.width / 2
            startAngle: root.control.startAngle - 90
            sweepAngle: root.control.position * (root.control.endAngle - root.control.startAngle)
        }
    }
}
