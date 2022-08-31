// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared as Examples

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("ColorAnimation", "Interpolates between colors",  Qt.resolvedUrl("basics/color-animation.qml"));
            addExample("PropertyAnimation", "Interpolates between numbers", Qt.resolvedUrl("basics/property-animation.qml"));
            addExample("Animators", "Using Animators",  Qt.resolvedUrl("basics/animators.qml"));
            addExample("Behaviors", "Animates procedural movement", Qt.resolvedUrl("behaviors/behavior-example.qml"));
            addExample("Wiggly Text", "Text that wiggles as you drag it", Qt.resolvedUrl("behaviors/wigglytext.qml"));
            addExample("Tv Tennis", "Paddles that follow a ball", Qt.resolvedUrl("behaviors/tvtennis.qml"));
            addExample("Easing Curves", "Compare available easing curves", Qt.resolvedUrl("easing/easing.qml"));
            addExample("States", "Simple states", Qt.resolvedUrl("states/states.qml"));
            addExample("Transitions", "Simple states with animated transitions", Qt.resolvedUrl("states/transitions.qml"));
            addExample("PathAnimation", "Animate along a path", Qt.resolvedUrl("pathanimation/pathanimation.qml"));
            addExample("PathInterpolator", "Interpolates along a path", Qt.resolvedUrl("pathinterpolator/pathinterpolator.qml"));
        }
    }
}
