// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 320
    height: 480
    visible: true
    title: qsTr("Shapes")

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
                { name: "Shape Gallery", description: "Simple path rendering examples", source: "shapegallery.qml" },
                { name: "Interactive Shape", description: "Dynamic, interactive path rendering examples", source: "interactive.qml" },
                { name: "Anti-aliasing", description: "Improving quality", source: "sampling.qml" },
                { name: "Magnify My Tiger!", description: "Path zooming example", source: "zoomtiger.qml" },
                { name: "Clip My Tiger!", description: "Clip examples, a.k.a. What Not To Do", source: "clippedtigers.qml" },
            ]
            onExampleClicked: (source) => stack.push(exampleLoader, { source })
        }

        Component {
            id: exampleLoader
            Loader {}
        }
    }
}
