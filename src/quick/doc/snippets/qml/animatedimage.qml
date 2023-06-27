// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// This snippet should be turned into an example and put in
// examples/quick/imageelements/animatedimage

//! [document]
import QtQuick

Rectangle {
    width: animation.width; height: animation.height + 8

    AnimatedImage { id: animation; source: "animation.gif" }

    Rectangle {
        property int frames: animation.frameCount

        width: 4; height: 8
        x: (animation.width - width) * animation.currentFrame / frames
        y: animation.height
        color: "red"
    }
}
//! [document]
