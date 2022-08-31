// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: root

    property alias textInput: textInput
    property alias text: textInput.text
    signal accepted
    signal downPressed
    implicitWidth: textInput.implicitWidth + rect.radius * 2
    implicitHeight: textInput.implicitHeight

    function copyAll() {
        textInput.selectAll()
        textInput.copy()
    }

    SystemPalette { id: palette }
    height: textInput.implicitHeight + 8
    clip: true

    Rectangle {
        id: rect
        anchors.fill: parent
        radius: height / 4
        color: palette.button
        border.color: Qt.darker(palette.button, 1.5)
    }

    TextInput {
        id: textInput
        color: palette.text
        anchors.fill: parent
        anchors.leftMargin: rect.radius
        anchors.rightMargin: rect.radius
        verticalAlignment: Text.AlignVCenter
        onAccepted: root.accepted()
        Keys.onDownPressed: root.downPressed()
    }
}
