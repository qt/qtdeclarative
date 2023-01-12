// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Fusion
import QtQuick.Controls.Fusion.impl

T.VerticalHeaderView {
    id: control

    implicitWidth: contentWidth
    implicitHeight: syncView ? syncView.height : 0

    delegate: Rectangle {
        // Qt6: add cellPadding (and font etc) as public API in headerview
        readonly property real cellPadding: 8

        implicitWidth: Math.max(control.width, text.implicitWidth + (cellPadding * 2))
        implicitHeight: text.implicitHeight + (cellPadding * 2)

        gradient: Gradient {
            GradientStop {
                position: 0
                color: Fusion.gradientStart(control.palette.button)
            }
            GradientStop {
                position: 1
                color: Fusion.gradientStop(control.palette.button)
            }
        }

        Label {
            id: text
            text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole]
                                        : model[control.textRole])
                                   : modelData
            width: parent.width
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
