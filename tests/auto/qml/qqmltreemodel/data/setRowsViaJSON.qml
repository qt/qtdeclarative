// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Qt.labs.qmlmodels

import "TreeData.js" as JsonData

Item {
    id: root
    width: 200
    height: 200

    property alias testModel: treeModel
    property alias treeView: treeView

    Component.onCompleted: treeView.model.rows = JsonData.folders

    TreeModel {
        id: treeModel
        objectName: "testModel"

        TableModelColumn { display: "checked" }
        TableModelColumn { display: "size" }
        TableModelColumn { display: "type" }
        TableModelColumn { display: "name" }
        TableModelColumn { display: "lastModified" }
    }

    TreeView {
        id: treeView
        anchors.fill: parent
        model: testModel
        delegate: Text {
            text: model.display
        }
    }
}
