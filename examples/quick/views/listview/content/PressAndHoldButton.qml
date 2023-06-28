// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Image {
    id: container

    property int repeatDelay: 300
    property int repeatDuration: 75
    property bool pressed

    signal clicked

    scale: pressed ? 0.9 : 1

    function release() {
        autoRepeatClicks.stop()
        container.pressed = false
    }

    SequentialAnimation on pressed {
        id: autoRepeatClicks
        running: false

        PropertyAction { target: container; property: "pressed"; value: true }
        ScriptAction { script: container.clicked() }
        PauseAnimation { duration: container.repeatDelay }

        SequentialAnimation {
            loops: Animation.Infinite
            ScriptAction { script: container.clicked() }
            PauseAnimation { duration: container.repeatDuration }
        }
    }

    MouseArea {
        anchors.fill: parent

        onPressed: autoRepeatClicks.start()
        onReleased: container.release()
        onCanceled: container.release()
    }
}

