// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.3
import QtQuick.Window 2.2

/*
    The test verifies that scaled down mipmapped images contains
    colors from all pixels.

    #samples: 2
                 PixelPos     R    G    B    Error-tolerance
    #final:        0   0     0.25 0.25 0.25        0.1
    #final:        1   0     0.25 0.25 0.25        0.1
*/

RenderTestBase
{
    Image {
        x: 0
        y: 0
        transformOrigin: Item.TopLeft
        source: "mipmap_small.png"
        mipmap: true
        smooth: false
        scale: 1 / (width * Screen.devicePixelRatio);
    }

    Image {
        x: 1
        y: 0
        transformOrigin: Item.TopLeft
        source: "mipmap_large.png"
        mipmap: true
        smooth: false
        scale: 1 / (width * Screen.devicePixelRatio);
    }

    onEnterFinalStage: finalStageComplete = true;
}
