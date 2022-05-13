// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2

Rectangle {
    id: box
    gradient: Gradient {
        GradientStop { position: 0.1; color: "red" }
        GradientStop { position: 0.9; color: "blue" }
    }
    width: 100
    height: 100
    anchors.centerIn: parent
    antialiasing: true

    property int rotationChangeCounter: 0
    onRotationChanged: ++rotationChangeCounter;

    property int scaleChangeCounter: 0
    onScaleChanged: ++scaleChangeCounter;

    property int opacityChangeCounter: 0
    onOpacityChanged: ++opacityChangeCounter

    property int xChangeCounter: 0;
    onXChanged: ++xChangeCounter;

    property int yChangeCounter: 0;
    onYChanged: ++yChangeCounter;

}
