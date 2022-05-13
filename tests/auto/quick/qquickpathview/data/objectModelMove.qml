// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml.Models 2.11
import QtQuick 2.11

Item {
    id: root
    width: 400
    height: 400
    visible: true

    property Item pathViewItem

    function destroyView() {
        if (pathViewItem)
            pathViewItem.destroy()
    }

    function newView() {
        pathViewItem = pathViewComponent.createObject(root)
    }

    function move() {
        objectModel.move(0, 1)
    }

    Component {
        id: pathViewComponent

        PathView {
            id: pathView
            objectName: "PathView"
            width: 32 * 3
            height: 32
            model: objectModel

            interactive: false
            snapMode: PathView.SnapToItem
            movementDirection: PathView.Positive
            highlightMoveDuration: 100

            path: Path {
                startX: 16
                startY: 16
                PathLine {
                    x: 16 + (32 * 3)
                    y: 16
                }
            }
        }
    }

    ObjectModel {
        id: objectModel

        Rectangle {
            objectName: "red"
            width: 32
            height: 32
            color: "red"
        }
        Rectangle {
            objectName: "green"
            width: 32
            height: 32
            color: "green"
        }
        Rectangle {
            objectName: "blue"
            width: 32
            height: 32
            color: "blue"
        }
    }
}
