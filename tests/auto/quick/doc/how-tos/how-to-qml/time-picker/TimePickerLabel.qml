// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

Item {
    id: root
    // Use TextMetrics' boundingRect.width rather than a RowLayout's implicitWidth
    // because the latter can cause TimePickerDialog to jump around when the label text changes.
    implicitWidth: fullTextMetrics.boundingRect.width + amPmLayout.implicitWidth + 80
    implicitHeight: fullTextMetrics.boundingRect.height

    property var time
    property bool am: true
    property bool showAmPm: true

    property bool hoursActive: true

    property int fontPixelSize: Qt.application.font.pixelSize * 4

    signal hoursSelected
    signal minutesSelected
    signal amSelected
    signal pmSelected

    TextMetrics {
        id: fullTextMetrics
        font: hoursLabel.font
        text: "99:99"
    }

    TextMetrics {
        id: hourTextMetrics
        font.family: hoursLabel.font.family
        font.pixelSize: hoursLabel.fontInfo.pixelSize
        text: "99"
    }

    TimeComponentLabel {
        id: hoursLabel
        objectName: "hoursLabel"
        text: Qt.formatTime(root.time, "hh")
        font.pixelSize: root.fontPixelSize
        // Avoid any jumping around by using a dedicated TextMetrics object
        // for our label too, and position ourselves based on its width.
        x: colonLabel.x - hourTextMetrics.boundingRect.width - 4
        anchors.verticalCenter: parent.verticalCenter
        dim: !root.hoursActive

        onTapped: root.hoursSelected()
    }

    TimeComponentLabel {
        id: colonLabel
        text: ":"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: root.fontPixelSize
        dim: true
    }

    TimeComponentLabel {
        id: minutesLabel
        objectName: "minutesLabel"
        text: Qt.formatTime(root.time, "mm")
        font.pixelSize: root.fontPixelSize
        anchors.left: colonLabel.right
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        dim: root.hoursActive

        onTapped: root.minutesSelected()
    }

    ColumnLayout {
        id: amPmLayout
        visible: root.showAmPm
        // We also need to avoid the AM/PM label jumping around,
        // so rather than anchor it to the right of the minutes label,
        // we use colonLabel's horizontal center (which never changes), and fullTextMetrics.
        anchors.left: colonLabel.horizontalCenter
        anchors.leftMargin: fullTextMetrics.boundingRect.width / 2 + 12
        anchors.verticalCenter: minutesLabel.verticalCenter

        spacing: 0

        ToolButton {
            objectName: "amButton"
            text: qsTr("AM")
            autoExclusive: true

            Material.foreground: Material.color(Material.Indigo,
                root.am ? Material.Shade500 : Material.Shade100)

            // Set the size explicitly to ensure that the buttons aren't too large.
            // We also add 1 to the width because we want square ripple effects, not round.
            Layout.preferredWidth: (implicitWidth * 0.7) + 1
            Layout.preferredHeight: (implicitHeight * 0.7)

            onClicked: {
                root.am = true
                root.amSelected()
            }
        }
        ToolButton {
            objectName: "pmButton"
            text: qsTr("PM")
            autoExclusive: true

            Material.foreground: Material.color(Material.Indigo,
                !root.am ? Material.Shade500 : Material.Shade100)

            Layout.preferredWidth: (implicitWidth * 0.7) + 1
            Layout.preferredHeight: (implicitHeight * 0.7)

            onClicked: {
                root.am = false
                root.pmSelected()
            }
        }
    }
}
//! [file]
