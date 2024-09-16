// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2

/*
    The test verifies that batching does not interfere with overlapping
    regions.

    #samples: 8
                 PixelPos     R    G    B    Error-tolerance
    #final:       10  10     0.0  0.0  0.0        0.0
    #final:       20  10     1.0  1.0  1.0        0.0
    #final:       30  10     0.0  0.0  0.0        0.0
    #final:       40  10     0.5  0.5  0.5        0.1
    #final:       50  10     0.0  0.0  0.0        0.0
    #final:       60  10     1.0  1.0  1.0        0.0
    #final:       70  10     0.0  0.0  0.0        0.0
    #final:       80  10     0.5  0.5  0.5        0.1
*/

RenderTestBase
{
    Item {
        x: 10
        y: 10
        scale: 10
        Image { x: 0; source: "blacknwhite.png"; smooth: false }
        Image { x: 2; source: "blacknwhite.png"; smooth: true }
        Image { x: 4; source: "blacknwhite.png"; smooth: false }
        Image { x: 6; source: "blacknwhite.png"; smooth: true }
    }

    finalStageComplete: true
}
