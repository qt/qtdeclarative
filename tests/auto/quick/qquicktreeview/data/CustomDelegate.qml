// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import TestModel

Rectangle {
    id: root

    implicitWidth: 100 // hard-coded to make it easier to test the layout
    implicitHeight: 25
    clip: true
    color: current || selected ? "lightgreen" : "white"

    property alias text: label.text

    readonly property real indent: 20
    readonly property real padding: 5

    // Assigned to by TreeView:
    required property TreeView treeView
    required property bool isTreeNode
    required property bool expanded
    required property int hasChildren
    required property int depth
    required property bool current
    required property bool selected

    TapHandler {
        onTapped: treeView.toggleExpanded(row)
    }

    Text {
        id: indicator
        visible: root.isTreeNode && root.hasChildren
        x: padding + (root.depth * root.indent)
        text: root.expanded ? "▼" : "▶"
    }

    Text {
        id: label
        x: padding + (root.isTreeNode ? (root.depth + 1) * root.indent : 0)
        width: root.width - root.padding - x
        clip: true
        text: model.display
    }
}
