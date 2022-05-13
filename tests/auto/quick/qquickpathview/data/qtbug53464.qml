// Copyright (C) 2016 Netris
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Rectangle {
    width: 600
    height: 400
    PathView {
        objectName: "pathView"
        model: 10
        anchors.fill: parent
        pathItemCount: 5
        highlightRangeMode: PathView.StrictlyEnforceRange
        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        currentIndex: 8

        path: Path {
            startX: 0
            startY: 50
            PathLine {
                x: 600
                y: 50
            }
        }

        delegate: Component {
            Text {
                width: 50
                height: 50
                text: index
                objectName: "delegate" + index
                font.pixelSize: 24
                color: PathView.isCurrentItem ? "green" : "black"
            }
        }
    }
}

