// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.9

Rectangle {
    implicitWidth: text.implicitWidth * 1.2
    implicitHeight: text.implicitHeight * 1.1
    color: "lightgrey"
    property color blipColor: "green"
    property int blipDuration: 30 // ms
    property alias label: text.text

    function blip() {
        blipAnim.start()
    }

    SequentialAnimation on color {
        id: blipAnim
        PropertyAction { value: blipColor }
        PauseAnimation { duration: blipDuration }
        PropertyAction { value: "lightgrey" }
    }

    Text {
        id: text
        anchors.centerIn: parent
    }
}
