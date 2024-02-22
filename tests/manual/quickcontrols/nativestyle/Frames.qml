// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "Frames"

    Row {
        spacing: container.rowSpacing

        Frame {
            width: 100
            height: 50
        }

        Frame {
            Rectangle {
                implicitWidth: label.width + 50
                implicitHeight: 45
                Label {
                    id: label
                    anchors.centerIn: parent
                    text: "Frame with contents"
                }
            }
        }
    }
}
