// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: slider.implicitWidth
    height: slider.implicitHeight
    visible: true

    property alias slider: slider

    Slider {
        id: slider
        stepSize: 0.2
        anchors.centerIn: parent

        Rectangle {
            anchors.fill: slider.handle
            radius: width / 2
            color: slider.pressed ? "#aa666666" : "transparent"
        }

        contentItem: Item {
            Repeater {
                id: repeater
                model: 6

                Rectangle {
                    x: ((slider.contentItem.width - slider.handle.width) * (index / (repeater.count - 1)))
                       - width / 2 + slider.handle.width / 2
                    y: parent.height
                    width: 1
                    height: 4
                    color: "#888"
                }
            }
        }
    }
}
