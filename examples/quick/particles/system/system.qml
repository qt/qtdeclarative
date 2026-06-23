// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 320
    height: 480
    visible: true
    title: qsTr("Particle System")

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
                { name: "Dynamic Comparison", description: "Compares with dynamically created elements", source: "dynamiccomparison.qml" },
                { name: "StartStop", description: "Start and stop the simulation", source: "startstop.qml" },
                { name: "Timed group changes", description: "Emit into managed groups", source: "timedgroupchanges.qml" },
                { name: "Dynamic Emitters", description: "Dynamically instantiated emitters with a single system", source: "dynamicemitters.qml" },
                { name: "Multiple Painters", description: "Several ParticlePainters on the same logical particles", source: "multiplepainters.qml" },
            ]
            onExampleClicked: (source) => stack.push(exampleLoader, { source })
        }

        Component {
            id: exampleLoader
            Loader {}
        }
    }
}
