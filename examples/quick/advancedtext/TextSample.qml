// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: textSample
    property bool textFits: textLabel.implicitWidth < statusRectangle.width
                            && statusRectangle.width - textLabel.implicitWidth < 20
    property alias text: textLabel.text
    property alias font: textLabel.font

    width: parent.width
    height: textLabel.height + controlsLayout.height
    anchors.margins: 10

    Rectangle {
        id: statusRectangle
        color: "transparent"
        border.color: textFits ? "forestgreen" : "firebrick"
        border.width: 2
        radius: 10
        anchors.fill: parent

        anchors.margins: -5
        antialiasing: true
    }

    Column {
        anchors.fill: parent
        id: column
        Label {
            id: textLabel
            font.family: georama.name
//! [variableAxes]
            font.variableAxes: {
                "wdth": widthSlider.value,
                "wght": weightSlider.value
            }
//! [variableAxes]
        }

        GridLayout {
            id: controlsLayout
            width: parent.width
            columns: 2
            Label {
                Layout.margins: 5
                text: qsTr("Width")
            }
            Slider {
                id: widthSlider
                from: 62.5
                to: 150
                value: 100
                Layout.fillWidth: true
                Layout.margins: 5
            }
            Label {
                text: qsTr("Weight")
                Layout.margins: 5
            }
            Slider {
                id: weightSlider
                from: 100
                to: 900
                value: 400
                Layout.fillWidth: true
                Layout.margins: 5
            }
        }
    }
}
