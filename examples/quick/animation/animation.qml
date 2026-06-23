// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 320
    height: 480
    visible: true
    title: qsTr("Animation")

    header: ToolBar {
        visible: stack.depth > 1
        height: visible ? implicitHeight : 0
        ToolButton {
            icon.source: "images/back.png"
            Accessible.name: qsTr("Back")
            onClicked: stack.pop()
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        focus: true
        initialItem: LauncherList {
            model: [
                { name: "ColorAnimation", description: "Interpolates between colors", source: "basics/color-animation.qml" },
                { name: "PropertyAnimation", description: "Interpolates between numbers", source: "basics/property-animation.qml" },
                { name: "Animators", description: "Using Animators", source: "basics/animators.qml" },
                { name: "Behaviors", description: "Animates procedural movement", source: "behaviors/behavior-example.qml" },
                { name: "Wiggly Text", description: "Text that wiggles as you drag it", source: "behaviors/wigglytext.qml" },
                { name: "Tv Tennis", description: "Paddles that follow a ball", source: "behaviors/tvtennis.qml" },
                { name: "Easing Curves", description: "Compare available easing curves", source: "easing/easing.qml" },
                { name: "States", description: "Simple states", source: "states/states.qml" },
                { name: "Transitions", description: "Simple states with animated transitions", source: "states/transitions.qml" },
                { name: "PathAnimation", description: "Animate along a path", source: "pathanimation/pathanimation.qml" },
                { name: "PathInterpolator", description: "Interpolates along a path", source: "pathinterpolator/pathinterpolator.qml" },
            ]
            onExampleClicked: (source) => stack.push(exampleLoader, { source })
        }

        Component {
            id: exampleLoader
            Loader {}
        }
    }
}
