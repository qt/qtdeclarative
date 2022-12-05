// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    Frame {
        width: parent.width - 60
        anchors.centerIn: parent
        topPadding: 12
        bottomPadding: 12

        RowLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                text: qsTr("A")
                font.pointSize: 15
                font.weight: 400
            }

            Slider {
                snapMode: Slider.SnapAlways
                stepSize: 1
                from: 15
                value: AppSettings.fontSize
                to: 21

                Layout.fillWidth: true

                onMoved: AppSettings.fontSize = value
            }

            Label {
                text: qsTr("A")
                font.pointSize: 21
                font.weight: 400
            }
        }
    }
}
