// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2

/*
    This test verifies that when we have an update to opacity above
    a batch root, the opacity of the batch root's children is rendered
    correctly. The Text element has 1000 glyphs in it, which is needed
    for contentRoot to become a batch root when the scale changes.

    #samples: 2
                 PixelPos     R    G    B    Error-tolerance
    #base:        50  50     0.0  0.0  1.0       0.0
    #final:       50  50     0.5  0.5  1.0       0.05
*/

RenderTestBase {
    id: root

    Item {
        id: failRoot;
        property alias itemScale: contentItem.scale

        Item {
            id: contentItem
            width: 100
            height: 100
            Rectangle {
                width: 100
                height: 100
                color: "blue"
                Text {
                    id: input
                    color: "black"
                    Component.onCompleted: { for (var i = 0; i<1000; ++i) input.text += 'x' }
                }
            }
        }
    }

    SequentialAnimation {
        id: unifiedAnimation;
        NumberAnimation { properties: "opacity,itemScale"; duration: 256; from: 1; to: 0.5; target: failRoot }
        ScriptAction { script: root.finalStageComplete = true; }
    }

    onEnterFinalStage: {
        unifiedAnimation.running = true;
    }

}
