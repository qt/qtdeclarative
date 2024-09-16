// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    width: 200
    height: 100
    gradient: Gradient {
        GradientStop { position: 0; color: "white" }
        GradientStop { position: 1; color: "black" }
    }
    Row {
        opacity: 0.5
        Item {
            id: foo
            width: 100; height: 100
            Rectangle { x: 5; y: 5; width: 60; height: 60; color: "red" }
            Rectangle { x: 20; y: 20; width: 60; height: 60; color: "orange" }
            Rectangle { x: 35; y: 35; width: 60; height: 60; color: "yellow" }
        }
        ShaderEffectSource {
            width: 100; height: 100
            sourceItem: foo
        }
        ShaderEffectSource {
            width: 100; height: 100
            sourceItem: foo
            recursive: true
            live: true
        }
    }
}
