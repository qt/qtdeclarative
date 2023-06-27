// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [0]
import QtQuick

Row {
    x: 10; y: 10
    spacing: 10

    Image { source: "pics/qt.png" }
    Image {
        source: "pics/qt.png"
        transform: Rotation { origin.x: 30; origin.y: 30; axis { x: 0; y: 1; z: 0 } angle: 18 }
    }
    Image {
        source: "pics/qt.png"
        transform: Rotation { origin.x: 30; origin.y: 30; axis { x: 0; y: 1; z: 0 } angle: 36 }
    }
    Image {
        source: "pics/qt.png"
        transform: Rotation { origin.x: 30; origin.y: 30; axis { x: 0; y: 1; z: 0 } angle: 54 }
    }
    Image {
        source: "pics/qt.png"
        transform: Rotation { origin.x: 30; origin.y: 30; axis { x: 0; y: 1; z: 0 } angle: 72 }
    }
}
//! [0]
