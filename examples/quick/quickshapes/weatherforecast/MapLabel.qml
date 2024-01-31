// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: mapLabel
    property url iconSource
    property string label
    property int degrees

    radius: 8 * mainWindow.designWindowWidthRatio
    color: Qt.rgba(0, 0, 0, 0.05)
    height: 42 * mainWindow.designWindowHeightRatio
    width: labelText.width + divider.width + labelDegrees.width + iconLoader.width + row.padding * 5

    x: -height / 2
    y: -height / 2

    Row {
        id: row
        anchors.fill: parent
        padding: 8 * mainWindow.designWindowWidthRatio
        spacing: padding

        Text {
            id: labelText
            text: mapLabel.label
            color: "#333"
            font.family: workSansRegular.font.family
            font.pixelSize: 22 * mainWindow.designWindowHeightRatio
            anchors.verticalCenter: parent.verticalCenter
        }

        Rectangle {
            id: divider
            color: "#333"
            width: 2 * mainWindow.designWindowWidthRatio
            antialiasing: true
            height: labelText.implicitHeight
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            id: labelDegrees
            text: degrees+"°"
            color: "#333"
            font.family: workSansRegular.font.family
            font.pixelSize: 22 * mainWindow.designWindowHeightRatio
            anchors.verticalCenter: parent.verticalCenter
        }

        Loader {
            id: iconLoader
            source: mapLabel.iconSource

            // TODO: implement PreserveAspectRatio in Shapes
            property real aspectRatio: implicitWidth / implicitHeight
            width: 25 * mainWindow.designWindowWidthRatio
            height: width / aspectRatio

            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
