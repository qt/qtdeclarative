// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: 100; height: 300

Item {
    x: 10; y: 10
    width: 80; height: 280

//! [rectangles]
Rectangle {
    y: 0; width: 80; height: 80
    color: "lightsteelblue"
}

Rectangle {
    y: 100; width: 80; height: 80
    gradient: Gradient {
        GradientStop { position: 0.0; color: "lightsteelblue" }
        GradientStop { position: 1.0; color: "blue" }
    }
}

Rectangle {
    y: 200; width: 80; height: 80
    rotation: 90
    gradient: Gradient {
        GradientStop { position: 0.0; color: "lightsteelblue" }
        GradientStop { position: 1.0; color: "blue" }
    }
}
//! [rectangles]

//! [presets]
Rectangle {
    y: 0; width: 80; height: 80
    gradient: Gradient.NightFade
}

Rectangle {
    y: 0; width: 80; height: 80
    gradient: "NightFade"
}
//! [presets]
}
}
