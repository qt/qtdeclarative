// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// Shared scene for render_HideFirstInBatch*.qml, see there.

RenderTestBase {
    id: root

    property string hideMode: "visible"

    // Changing the source rebuilds the batches, which sorts them again
    Image {
        id: image
        y: 190; width: 60; height: 10
        source: "widebtn1.png"
    }

    // Smooth color material, first element of the batch it shares with blueRect
    Rectangle {
        id: greenRect
        x: 10; y: 10; width: 180; height: 30
        antialiasing: true
        color: "#00ff00"
    }

    // Flat vertex color material, does not overlap blueRect
    Rectangle {
        id: yellowRect
        x: 10; y: 50; width: 50; height: 30
        color: Qt.rgba(1, 1, 0, 0.99)
    }

    // Smooth color material
    Rectangle {
        id: blueRect
        x: 80; y: 100; width: 100; height: 80
        antialiasing: true
        color: "#0000ff"
    }

    // Flat vertex color material, on top of blueRect
    Rectangle {
        id: redRect
        x: 100; y: 120; width: 40; height: 40
        color: Qt.rgba(1, 0, 0, 0.99)
    }

    ShaderEffectSource {
        id: effectSource
        width: 0; height: 0
        sourceItem: root.hideMode === "hideSource" ? greenRect : null
    }

    onEnterFinalStage: {
        if (root.hideMode === "opacity")
            greenRect.opacity = 0;
        else if (root.hideMode === "hideSource")
            effectSource.hideSource = true;
        else
            greenRect.visible = false;
        image.source = "widebtn2.png";
        finalStageComplete = true;
    }
}
