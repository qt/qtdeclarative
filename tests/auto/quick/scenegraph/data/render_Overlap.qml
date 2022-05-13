// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2

/*
    The test verifies that batching does not interfere with overlapping
    regions.

    #samples: 4
                 PixelPos     R    G    B    Error-tolerance
    #base:        20  20     0.0  0.0  0.0        0.0
    #base:        30  30     0.5  0.0  0.0        0.05
    #final:       50  50     0.0  0.0  0.0        0.0
    #final:       60  60     0.5  0.0  0.0        0.05
*/

RenderTestBase
{
    Rectangle {
        x: 20
        y: 20
        width: 80
        height: 80
        color: "black"

        Rectangle {
            anchors.fill: parent
            anchors.margins: 10
            color: "red"
            opacity: 0.5
        }
    }

    Rectangle {
        x: 50
        y: 50
        width: 80
        height: 80
        color: "black"
        Rectangle {
            anchors.fill: parent
            anchors.margins: 10
            color: "red"
            opacity: 0.5
        }
    }

    finalStageComplete: true
}
