// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.14
import QtQuick.Shapes 1.14
import Qt.test 1.0

Shape {
    width: 200
    height: 150
    vendorExtensionsEnabled: false
    objectName: "shape"
    id: shape
    property alias paths: multiline.paths
    property point vertexBeingChecked;

    function checkVertexAt(i, j) {
        vertexBeingChecked = multiline.paths[i][j]
    }

    PolygonProvider {
        id: provider
        objectName: "provider"
    }

    ShapePath {
        strokeWidth: 4
        strokeColor: "green"
        PathMultiline {
            id: multiline
            paths: provider.paths
        }
    }
}
