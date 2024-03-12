// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2

/*
    The test verifies that batches of translucent items get split
    into multiple batches when an item in them change opacity.

                 PixelPos     R    G    B    Error-tolerance
    #base:         0   0     1.0  0.5  0.5        0.05
    #base:       100   0     0.5  0.5  1.0        0.05
    #base:         0 100     1.0  0.5  0.5        0.05
    #base:       100 100     0.5  0.5  1.0        0.05
    #final:        0   0     1.0  0.5  0.5        0.05
    #final:      100   0     0.1  0.1  1.0        0.05
    #final:        0 100     1.0  0.5  0.5        0.05
    #final:      100 100     0.9  0.9  1.0        0.05

    #samples: 8
*/

RenderTestBase {

    Item {
        Rectangle { id: redUnclipped;  opacity: 0.5; width: 50; height: 50; color: "red" }
        Rectangle { id: blueUnclipped; opacity: 0.5; width: 50; height: 50; color: "blue"; x: 100 }
    }

    Item {
        clip: true;
        y: 100
        width: 200
        height: 100
        Rectangle { id: redClipped;  opacity: 0.5; width: 50; height: 50; color: "red" }
        Rectangle { id: blueClipped; opacity: 0.5; width: 50; height: 50; color: "blue"; x: 100 }
    }

    onEnterFinalStage: {
        blueUnclipped.opacity = 0.9
        blueClipped.opacity = 0.1
        finalStageComplete = true;
    }

}
