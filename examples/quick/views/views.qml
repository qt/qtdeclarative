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
            addExample("GridView", "A simple GridView", Qt.resolvedUrl("gridview/gridview-example.qml"))
            addExample("Dynamic List", "A dynamically alterable list", Qt.resolvedUrl("listview/dynamiclist.qml"))
            addExample("Expanding Delegates", "A ListView with delegates that expand", Qt.resolvedUrl("listview/expandingdelegates.qml"))
            addExample("Highlight", "A ListView with a custom highlight", Qt.resolvedUrl("listview/highlight.qml"))
            addExample("Highlight Ranges", "The three highlight ranges of ListView", Qt.resolvedUrl("listview/highlightranges.qml"))
            addExample("Sections", "ListView section headers and footers", Qt.resolvedUrl("listview/sections.qml"))
            addExample("Packages", "Transitions between a ListView and GridView", Qt.resolvedUrl("package/view.qml"))
            addExample("PathView", "A simple PathView", Qt.resolvedUrl("pathview/pathview-example.qml"))
            addExample("ObjectModel", "Using an ObjectModel", Qt.resolvedUrl("objectmodel/objectmodel.qml"))
            addExample("Display Margins", "A ListView with display margins", Qt.resolvedUrl("listview/displaymargin.qml"))
            addExample("Draggable Selections", "Enabling drag-and-drop on DelegateModel delegates", Qt.resolvedUrl("delegatemodel/dragselection.qml"))
        }
    }
}
