/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the manual tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
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
