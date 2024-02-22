// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import TestModel
import QtQuick.Controls

Item {
    width: 800
    height: 600

    property alias treeView: treeView
    property alias selectionRectangle: selectionRectangle

    TreeView {
        id: treeView
        anchors.fill:parent
        anchors.margins: 10
        model: TestModel {}
        clip: true

        delegate: TreeViewDelegate {}
        selectionModel: ItemSelectionModel { model: treeView.model }
    }

    SelectionRectangle {
        id: selectionRectangle
        target: treeView
    }
}
