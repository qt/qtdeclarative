// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "Sliders normal"
    property int sliderWidth: 300
    property int sliderHeight: 140

    Row {
        spacing: 40

        Column {
            spacing: 15

            Slider {
                width: sliderWidth
                from: 0
                to: 10
                value: 5
            }

            Slider {
                width: sliderWidth
                from: 0
                to: 10
                value: 5
                enabled: false
            }

            Slider {
                width: sliderWidth
                from: 0
                to: 100
                value: 20
                stepSize: 20

                property int qqc2_style_tickPosition: 1
            }

            Slider {
                width: sliderWidth
                from: 0
                to: 100
                stepSize: 5
                value: 65

                property int qqc2_style_tickPosition: 2
            }

            Slider {
                // Should show 9 tickmarks
                width: sliderWidth
                from: 3.3
                to: 3.7
                stepSize: 0.05
                value: 3.5
                property int qqc2_style_tickPosition: 3
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
            }

            Slider {
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 10
                value: 5
                enabled: false
            }

            Slider {
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 100
                value: 20
                stepSize: 20

                property int qqc2_style_tickPosition: 1
            }

            Slider {
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 100
                stepSize: 5
                value: 65

                property int qqc2_style_tickPosition: 2
            }
            Slider {
                // Should show 9 tickmarks
                height: sliderHeight
                orientation: Qt.Vertical
                from: 3.3
                to: 3.7
                stepSize: 0.05
                value: 3.5
                property int qqc2_style_tickPosition: 3
            }
        }
    }
}
