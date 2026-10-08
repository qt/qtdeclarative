// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

/*
    QTBUG-85965, QTBUG-151316.

    All four rectangles are in the alpha render list. The green and the blue
    one share a material and end up in one batch, with the green rectangle
    first. The red rectangle shares a material with the yellow one, but
    overlaps the blue rectangle, so it must not join the yellow rectangle's
    batch, as that batch is rendered before the blue one.

    Hide the green rectangle, removing the first element of its batch, and
    at the same time change an unrelated Image, so that the batches get
    rebuilt and sorted again. Verify that the red rectangle is still on top
    of the blue one.

    Here the green rectangle is hidden by setting visible to false.

    #samples: 4
                 PixelPos     R         G          B           Error-tolerance
    #base:       100   25    0.0       1.0        0.0          0.05
    #base:       120  140    0.99      0.0        0.01         0.05
    #final:      100   25    1.0       1.0        1.0          0.05
    #final:      120  140    0.99      0.0        0.01         0.05
*/

HideFirstInBatchScene {
    hideMode: "visible"
}
