// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick 2.15
import Qt.labs.qmlmodels 1.0

TableView {
    id: root
    width: 1024
    height: 200
    property int tileSize: 1024 / 8
    rowSpacing: 1; columnSpacing: 1
    model: TableModel {
        TableModelColumn {}
        TableModelColumn {}
        TableModelColumn {}
        TableModelColumn {}
        TableModelColumn {}
        TableModelColumn {}
        TableModelColumn {}
        TableModelColumn {}
        columns: [0, 1, 2, 3, 4, 5, 6, 7]
        rows: [0, 1, 2, 3]
    }
    delegate: Image {
        id: image
//        source: "map.png"  // unscaled because png doesn't support QImageIOHandler::ScaledClipRect
//        source: "map.svgz" // tiles offset incorrectly: see QTBUG-81044
        source: "map.pdf"  // ok if the qtpdf plugin is installed
        fillMode: Image.Pad
        sourceSize.width: 1024
        sourceClipRect: Qt.rect(model.column * tileSize, model.row * tileSize, tileSize, tileSize)
        cache: true
        asynchronous: true
        Text {
            text: image.sourceClipRect.width + "x" + image.sourceClipRect.height +
                  " " + image.sourceClipRect.x + "," + image.sourceClipRect.y +
                  "\nfrom " + image.sourceSize.toString() + // inconsistency: if we don't set sourceSize, it ends up being sourceClipRect.size
                  "\nimplicit " + image.implicitWidth + "x" + image.implicitHeight
        }
    }
    columnWidthProvider: function (c) { return tileSize } // workaround for QTBUG-81045
    rowHeightProvider: function (r) { return tileSize }
}
