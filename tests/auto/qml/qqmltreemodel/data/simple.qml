// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Qt.labs.qmlmodels

Item {
    id: root
    width: 200
    height: 200

    property alias testModel: treeModel
    property alias treeView: treeView

    TreeModel {
        id: treeModel
        objectName: "testModel"

        TableModelColumn { display: "treeIndex" }
        TableModelColumn { display: "childIndex" }  // index in the vector of children

        rows: [
            {
                treeIndex: "[0]",
                childIndex: 0,
                rows: [
                    {
                        treeIndex: "[0,0]",
                        childIndex: 0
                    },
                    {
                        treeIndex: "[0,1]",
                        childIndex: 1,
                        rows: [
                            {
                                treeIndex: "[0,1,0]",
                                childIndex: 0
                            },
                            {
                                treeIndex: "[0,1,1]",
                                childIndex: 1
                            },
                            {
                                treeIndex: "[0,1,2]",
                                childIndex: 2
                            },
                            {
                                treeIndex: "[0,1,3]",
                                childIndex: 3
                            },
                            {
                                treeIndex: "[0,1,4]",
                                childIndex: 4
                            },
                            {
                                treeIndex: "[0,1,5]",
                                childIndex: 5
                            }
                        ]
                    },
                    {
                        treeIndex: "[0,2]",
                        childIndex: 2
                    },
                    {
                        treeIndex: "[0,3]",
                        childIndex: 3
                    },
                    {
                        treeIndex: "[0,4]",
                        childIndex: 4
                    }
                ]
            },
            {
                treeIndex: "[1]",
                childIndex: 1,
                rows: [
                    {
                        treeIndex: "[1,0]",
                        childIndex: 0
                    },
                    {
                        treeIndex: "[1,1]",
                        childIndex: 1
                    },
                    {
                        treeIndex: "[1,2]",
                        childIndex: 2
                    },
                    {
                        treeIndex: "[1,3]",
                        childIndex: 3
                    },
                    {
                        treeIndex: "[1,4]",
                        childIndex: 4,
                        rows: [
                            {
                                treeIndex: "[1,4,0]",
                                childIndex: 0
                            },
                            {
                                treeIndex: "[1,4,1]",
                                childIndex: 1
                            },
                            {
                                treeIndex: "[1,4,2]",
                                childIndex: 2
                            },
                            {
                                treeIndex: "[1,4,3]",
                                childIndex: 3
                            },
                            {
                                treeIndex: "[1,4,4]",
                                childIndex: 4
                            },
                            {
                                treeIndex: "[1,4,5]",
                                childIndex: 5
                            },
                            {
                                treeIndex: "[1,4,6]",
                                childIndex: 6
                            },
                            {
                                treeIndex: "[1,4,7]",
                                childIndex: 7
                            },
                            {
                                treeIndex: "[1,4,8]",
                                childIndex: 8
                            },
                            {
                                treeIndex: "[1,4,9]",
                                childIndex: 9
                            }
                        ]
                    }
                ]
            }
        ]
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
