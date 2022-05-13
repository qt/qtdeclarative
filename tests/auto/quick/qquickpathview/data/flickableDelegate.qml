// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

PathView {
    id: pathView
    objectName: "pathView"
    width: 400
    height: 200

    dragMargin: 400
    preferredHighlightBegin: 0.3
    preferredHighlightEnd: 0.3

    model: 3
    path: Path {
        startX: -pathView.width/2
        startY: pathView.height / 2
        PathLine { x: pathView.width + pathView.width/2; y: pathView.height / 2 }
    }

    delegate: Flickable {
        clip: true
        flickableDirection: Qt.Horizontal
        width: 200; height: 200
        contentWidth: 400
        contentX: 100
        Rectangle {
            y: 50
            x: 50
            width: 300
            height: 100
            color: "purple"
        }
Text { text: index }
        MouseArea {
            anchors.fill: parent
        }
    }
}
