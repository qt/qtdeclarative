// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2

/*
    This test verifies that items that go from being batched because
    of no overlap will be split into multiple batches because of an
    overlap and that no rendering errors occur as a result of this.

    #samples: 3
                 PixelPos     R    G    B    Error-tolerance
    #final:       10  10     1.0  0.0  0.0       0.05
    #final:       10 110     0.0  1.0  0.0       0.05
    #final:       10 120     0.0  0.0  1.0       0.05
*/

RenderTestBase {
    Item {
        opacity: 0.99; // Just to trigger alpha batches
        Rectangle { color: "#ff0000";  x: 10;  y: 10;  width: 20;  height: 20; }
        Image { source: "logo-small.jpg";  x: 10;  y: 50;   width: 50; height: 51; }
        Rectangle { color: "#00ff00"; x: 10;  y: 100; width: 50; height: 50; }
        Rectangle { color: "#0000ff"; x: 10;  y: 120; width: 10; height: 10; }
    }
    onEnterFinalStage: finalStageComplete = true;
}
