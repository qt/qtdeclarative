// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// Steps the scene once per frame, then lets the grabber take the image
QtObject {
    id: stepper
    required property Item scene
    property int frames: 30
    property int frame: 0
    readonly property bool running: frame < frames

    property Connections conn: Connections {
        target: stepper.scene.Window.window
        function onAfterAnimating() {
            if (stepper.frame < stepper.frames) {
                // Only the software backend paints partially, others need just the end state
                if (stepper.scene.GraphicsInfo.api !== GraphicsInfo.Software)
                    stepper.frame = stepper.frames - 1
                stepper.scene.step(stepper.frame)
                ++stepper.frame
                stepper.scene.Window.window.update()
            }
        }
    }
}
