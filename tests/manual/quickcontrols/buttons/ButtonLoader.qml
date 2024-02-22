// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

Item {
    id: root
    implicitWidth: activeButton.implicitWidth
    implicitHeight: activeButton.implicitHeight

    property bool round: false

    property string text
    property bool flat
    property bool hoverEnabled
    property bool highlighted
    property bool checked
    property var down: undefined

    property T.AbstractButton activeButton: round ? roundButton : button

    Button {
        id: button
        visible: !round
        text: root.text
        flat: root.flat
        hoverEnabled: root.hoverEnabled
        highlighted: root.highlighted
        checked: root.checked
        down: root.down
        enabled: root.enabled
    }

    RoundButton {
        id: roundButton
        visible: round
        text: "\u2713"
        flat: root.flat
        hoverEnabled: root.hoverEnabled
        highlighted: root.highlighted
        checked: root.checked
        down: root.down
        enabled: root.enabled

        Label {
            text: root.text
            font.pixelSize: roundButton.font.pixelSize * 0.5
            anchors.top: parent.bottom
            anchors.topMargin: 2
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
