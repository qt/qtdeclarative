// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.SplitView {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    handle: Rectangle {
        implicitWidth: control.orientation === Qt.Horizontal ? 1 : control.width
        implicitHeight: control.orientation === Qt.Horizontal ? control.height : 1
        color: control.palette.mid
        // Increase the hit area
        containmentMask: Item {
            width: control.orientation === Qt.Horizontal ? 8 : control.width
            height: control.orientation === Qt.Horizontal ? control.height : 8
        }
    }
}

