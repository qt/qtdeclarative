// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

/*
    The test creates four rectangles and checks that updates are still reflected in the rendered
    output regardless of mutability group.

    #samples: 8
                 PixelPos     R    G    B    Error-tolerance
    #base:        20  20     1.0  0.0  0.0        0.05
    #base:        20  60     1.0  0.0  0.0        0.05
    #base:        90 100     1.0  1.0  1.0        0.05
    #base:        20 140     1.0  0.0  0.0        0.05
    #final:       20  20     1.0  0.0  0.0        0.05
    #final:       20  60     0.0  1.0  0.0        0.05
    #final:       90 100     0.0  0.0  1.0        0.05
    #final:       20 140     1.0  1.0  0.0        0.05
*/

RenderTestBase
{
    id: root
    width: 200
    height: 200

    // Static mutability group - these should batch together
    Rectangle {
        id: static1
        x: 10
        y: 10
        width: 80
        height: 30
        color: "red"
        mutabilityGroup: Item.StaticMutabilityGroup
    }

    Rectangle {
        id: static2
        x: 10
        y: 50
        width: 80
        height: 30
        color: "red"
        mutabilityGroup: Item.StaticMutabilityGroup
    }

    // Dynamic mutability group - should NOT batch with static items
    Rectangle {
        id: dynamic
        x: 10
        y: 90
        width: 80
        height: 30
        color: "blue"
        mutabilityGroup: Item.DynamicMutabilityGroup
    }

    // Another static item
    Rectangle {
        id: static3
        x: 10
        y: 130
        width: 80
        height: 30
        color: "red"
        mutabilityGroup: Item.StaticMutabilityGroup
    }

    onEnterFinalStage: {
        static2.color = Qt.rgba(0, 1, 0, 1);
        dynamic.x = 20;
        static3.mutabilityGroup = Item.DynamicMutabilityGroup
        static3.color = Qt.rgba(1, 1, 0, 1)

        finalStageComplete = true;
    }
}
