// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2

/*
    The test verifies that batching does not interfere with overlapping
    regions.

    #samples: 8
                 PixelPos     R    G    B    Error-tolerance
    #base:        10  10     1.0  0.0  0.0        0.0
    #base:        10 110     1.0  0.0  0.0        0.0
    #base:        10  11     0.0  0.0  1.0        0.0
    #base:        10 111     0.0  0.0  1.0        0.0

    #final:       10  10     1.0  0.0  0.0        0.05
    #final:       10 110     1.0  0.0  0.0        0.05
    #final:       10  11     0.0  0.0  1.0        0.05
    #final:       10 111     0.0  0.0  1.0        0.05
*/

RenderTestBase
{
    id: root
    property real offset: 0;
    property real farAway: 10000000;

    Item {
        y: -root.offset + 10
        x: 10
        Repeater {
            model: 200
            Rectangle {
                x: index % 100
                y: root.offset + (index < 100 ? 0 : 1);
                width: 1
                height: 1
                color: index < 100 ? "red" : "blue"
                antialiasing: true;
            }
        }
    }

    Item {
        y: -root.offset + 110
        x: 10
        Item {
            y: root.offset

            Repeater {
                model: 200
                Rectangle {
                    x: index % 100
                    y: (index < 100 ? 0 : 1);
                    width: 1
                    height: 1
                    color: index < 100 ? "red" : "blue"
                    antialiasing: true;
                }
            }
        }
    }

    onEnterFinalStage: {
        root.offset = root.farAway;
        root.finalStageComplete = true;
    }

}
