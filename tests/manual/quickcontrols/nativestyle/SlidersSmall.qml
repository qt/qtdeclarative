// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "Sliders small"
    property int sliderWidth: 200
    property int sliderHeight: 100

    Row {
        spacing: 40

        Column {
            spacing: 15

            Slider {
                width: sliderWidth
                from: 0
                to: 10
                value: 5
                property bool qqc2_style_small
            }

            Slider {
                width: sliderWidth
                from: 0
                to: 10
                value: 5
                enabled: false
                property bool qqc2_style_small
            }

            Slider {
                width: sliderWidth
                from: 0
                to: 100
                value: 20
                stepSize: 20

                property bool qqc2_style_small
                property int qqc2_style_tickPosition: 1
            }

            Slider {
                width: sliderWidth
                from: 0
                to: 100
                stepSize: 5
                value: 65

                property bool qqc2_style_small
                property int qqc2_style_tickPosition: 2
            }
        }

        Row {
            spacing: 20

            Slider {
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 10
                value: 5
                property bool qqc2_style_small
            }

            Slider {
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 10
                value: 5
                enabled: false
                property bool qqc2_style_small
            }

            Slider {
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 100
                value: 20
                stepSize: 20

                property bool qqc2_style_small
                property int qqc2_style_tickPosition: 1
            }

            Slider {
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 100
                stepSize: 5
                value: 65

                property bool qqc2_style_small
                property int qqc2_style_tickPosition: 2
            }
        }
    }
}
