// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Grid {
    width: 200; height: 400
    flow: Grid.TopToBottom
    rows: 4
    leftPadding: 10; rightPadding: 10; topPadding: 10; bottomPadding: 10
    columnSpacing: 20; rowSpacing: 20

//! [rectangles]
Rectangle {
    width: 80; height: 80
    color: "lightsteelblue"
}

Rectangle {
    width: 80; height: 80
    gradient: Gradient {
        GradientStop { position: 0.0; color: "lightsteelblue" }
        GradientStop { position: 1.0; color: "blue" }
    }
}

Rectangle {
    width: 80; height: 80
    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "lightsteelblue" }
        GradientStop { position: 1.0; color: "blue" }
    }
}

Rectangle {
    width: 80; height: 80
    rotation: 90
    gradient: Gradient {
        GradientStop { position: 0.0; color: "lightsteelblue" }
        GradientStop { position: 1.0; color: "blue" }
    }
}
//! [rectangles]

//! [presets]
Rectangle {
    width: 80; height: 80
    gradient: Gradient.NightFade
}

Rectangle {
    width: 80; height: 80
    gradient: "SunnyMorning"
}
//! [presets]
}
