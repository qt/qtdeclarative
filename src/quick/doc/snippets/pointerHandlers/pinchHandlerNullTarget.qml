// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Window {
    width: 320; height: 240
    visible: true
    title: handler.persistentRotation.toFixed(1) + "° " +
           handler.persistentTranslation.x.toFixed(1) + ", " +
           handler.persistentTranslation.y.toFixed(1) + " " +
           (handler.persistentScale * 100).toFixed(1) + "%"

    PinchHandler {
        id: handler
        target: null
        persistentScale: 0.25
        onTranslationChanged: (delta) => {
            image.x -= delta.x
            image.y -= delta.y
        }
    }

    Image {
        id: image
        source: "images/album-cover.jpg"
        scale: handler.persistentScale
        x: -600; y: -450
    }
}
//![0]
