// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 320
    height: 480
    visible: true
    title: qsTr("Affectors")

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
                { name: "Age", description: "Kills off particles that enter the affector", source: "age.qml" },
                { name: "Attractor", description: "Simulates a small black hole", source: "attractor.qml" },
                { name: "Custom Affector", description: "Custom falling leaves", source: "customaffector.qml" },
                { name: "Friction", description: "Leaves that slow down as they fall", source: "friction.qml" },
                { name: "Gravity", description: "Leaves that fall towards the earth as you move it", source: "gravity.qml" },
                { name: "GroupGoal", description: "Balls that can be set on fire various ways", source: "groupgoal.qml" },
                { name: "Move", description: "Some effects you can get by altering trajectory midway", source: "move.qml" },
                { name: "SpriteGoal", description: "A ship that makes asteroids explode", source: "spritegoal.qml" },
                { name: "Turbulence", description: "A candle with faint wind", source: "turbulence.qml" },
                { name: "Wander", description: "Drifting snow flakes", source: "wander.qml" },
            ]
            onExampleClicked: (source) => stack.push(exampleLoader, { source })
        }

        Component {
            id: exampleLoader
            Loader {}
        }
    }
}
