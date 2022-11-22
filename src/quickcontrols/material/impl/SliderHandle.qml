// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl

Item {
    id: root
    implicitWidth: initialSize
    implicitHeight: initialSize

    property real value: 0
    property bool handleHasFocus: false
    property bool handlePressed: false
    property bool handleHovered: false
    readonly property int initialSize: 13
    readonly property var control: parent

    Rectangle {
        id: handleRect
        width: parent.width
        height: parent.height
        radius: width / 2
        color: root.control
            ? root.control.enabled ? root.control.Material.accentColor : root.control.Material.sliderDisabledColor
            : "transparent"
    }

    Ripple {
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: 22; height: 22
        pressed: root.handlePressed
        active: root.handlePressed || root.handleHasFocus || (enabled && root.handleHovered)
        color: root.control ? root.control.Material.highlightedRippleColor : "transparent"
    }

    Rectangle {
        anchors.bottom: parent.top
        anchors.bottomMargin: 6
        anchors.horizontalCenter: parent.horizontalCenter
        scale: root.handlePressed ? 1 : 0
        implicitWidth: Math.max(tm.width + 8, height)
        implicitHeight: tm.height + 8
        radius: height / 2
        color: root.control ? root.control.Material.accentColor : "transparent"
        transformOrigin: Item.Bottom

        Behavior on scale {
            NumberAnimation {
                duration: 100
            }
        }

        Rectangle {
            anchors.top: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            // Center the square to the parent circle, then position it downwards by half of the diagonal
            anchors.topMargin: Math.floor(-parent.radius * 1.5 + Math.sqrt(parent.radius * parent.radius / 2))
            implicitWidth: parent.radius
            implicitHeight: parent.radius
            rotation: 45
            color: root.control ? root.control.Material.accentColor : "transparent"
        }

        TextMetrics {
            id: tm
            text: '8'.repeat(label.text.length)
            font: label.font
        }

        Text {
            id: label
            anchors.centerIn: parent
            text: Math.abs(Math.floor(value) - value.toFixed(2)) < Number.EPSILON ? Math.trunc(value) : value.toFixed(2)
            color: root.control ? root.control.Material.primaryHighlightedTextColor : "transparent"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
