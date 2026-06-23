// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 320
    height: 480
    visible: true
    title: qsTr("Views")

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
                { name: "GridView", description: "A simple GridView", source: "gridview/gridview-example.qml" },
                { name: "Dynamic List", description: "A dynamically alterable list", source: "listview/dynamiclist.qml" },
                { name: "Expanding Delegates", description: "A ListView with delegates that expand", source: "listview/expandingdelegates.qml" },
                { name: "Highlight", description: "A ListView with a custom highlight", source: "listview/highlight.qml" },
                { name: "Highlight Ranges", description: "The three highlight ranges of ListView", source: "listview/highlightranges.qml" },
                { name: "Sections", description: "ListView section headers and footers", source: "listview/sections.qml" },
                { name: "Packages", description: "Transitions between a ListView and GridView", source: "package/view.qml" },
                { name: "PathView", description: "A simple PathView", source: "pathview/pathview-example.qml" },
                { name: "ObjectModel", description: "Using an ObjectModel", source: "objectmodel/objectmodel.qml" },
                { name: "Display Margins", description: "A ListView with display margins", source: "listview/displaymargin.qml" },
                { name: "Draggable Selections", description: "Enabling drag-and-drop on DelegateModel delegates", source: "delegatemodel/dragselection.qml" },
            ]
            onExampleClicked: (source) => stack.push(exampleLoader, { source })
        }

        Component {
            id: exampleLoader
            Loader {}
        }
    }
}
