// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.14
import tst_qquickpathitem 1.0

Item {
    id: item
    width: 200
    height: 150

    Shape {
        vendorExtensionsEnabled: false
        objectName: "shape"
        id: shape
        anchors.fill: parent

        ShapePath {
            strokeWidth: 4
            strokeColor: "red"
            scale: Qt.size(shape.width - 1, shape.height - 1)
            fillGradient: LinearGradient {
                x1: 20; y1: 20
                x2: 180; y2: 130
                GradientStop { position: 0; color: "blue" }
                GradientStop { position: 0.2; color: "green" }
                GradientStop { position: 0.4; color: "red" }
                GradientStop { position: 0.6; color: "yellow" }
                GradientStop { position: 1; color: "cyan" }
            }
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 4 ]
            startX: 20; startY: 20 // unnecessary, PathPolyline moves to the first vertex.
            PathPolyline {
                path: [ Qt.point(20.0  / (item.width - 1.0), 20.0  / (item.height - 1.0)),
                        Qt.point(180.0 / (item.width - 1.0), 130.0 / (item.height - 1.0)),
                        Qt.point(20.0  / (item.width - 1.0), 130.0 / (item.height - 1.0)),
                        Qt.point(20.0  / (item.width - 1.0), 20.0  / (item.height - 1.0)) ]
            }
        }
    }
}
