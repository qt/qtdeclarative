// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 320
    height: 480
    visible: true
    title: qsTr("Text")

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
                { name: "Hello", description: "An Animated Hello World", source: "fonts/hello.qml" },
                { name: "Fonts", description: "Using various fonts with a Text element", source: "fonts/fonts.qml" },
                { name: "Available Fonts", description: "A list of your available fonts", source: "fonts/availableFonts.qml" },
                { name: "Banner", description: "Large, scrolling text", source: "fonts/banner.qml" },
                { name: "Img tag", description: "Embedding images into text", source: "imgtag/imgtag.qml" },
                { name: "Text Layout", description: "Flowing text around items", source: "styledtext-layout.qml" },
            ]
            onExampleClicked: (source) => stack.push(exampleLoader, { source })
        }

        Component {
            id: exampleLoader
            Loader {}
        }
    }
}
