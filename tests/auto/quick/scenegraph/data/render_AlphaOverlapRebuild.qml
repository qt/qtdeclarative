// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

/*
    QTBUG-92984.

    Have three Image (with semi-transparency) and two semi-transparent
    Rectangle elements, and so all in the alpha render list, with images that
    are big enough to not get atlased. (meaning the underlying nodes never get
    merged, but the nodes for the Rectangle elements might)

    Lay them out vertically below each other, with the two Rectangles on top of
    the second and third Images, respectively. Then change (swap) the source
    property of the Images. This triggers a rebuild in the batch renderer.

    Verify that the results are still correct, i.e. that the two Rectangle
    elements do not get merged. An incorrect result would be having the third
    Image rendered on top of the corresponding Rectangle due the two Rectangles
    (incorrectly) being in one merged batch. The Image should always be below,
    regardless of which nodes get changed, invalidated, and how batches get
    rebuilt.

    The base-final sample set 1 just verifies that the Image changes from the
    bluish to greenish. The important part is the second set of samples: this
    checks that the red(ish) rectangle is still on top of the third Image. With
    incorrect merging behavior the second final result would be the same as the
    first final one (i.e. the "background" Image rendered, incorrectly, on top
    of the Rectangle).

    #samples: 4
                 PixelPos     R         G          B           Error-tolerance
    #base:        30  115    0.24313   0.30588    0.99607       0.05
    #base:        30  124    0.847059  0.062745   0.2           0.05
    #final:       30  115    0.36078   0.99607    0.42745       0.05
    #final:       30  124    0.870588  0.2        0.0862745     0.05
*/

RenderTestBase {
    id: root

    property string selectedItem: "item2"

    Item {
        width: 150; height: 50
        Image {
            width: parent.width
            objectName: "item1"
            source: "widebtn1.png"
        }
    }

    Item {
        y: 50; width: 150; height: 50
        Image {
            width: parent.width
            objectName: "item2"
            source: selectedItem == objectName ? "widebtn2.png" : "widebtn1.png"
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: 20
            color: "red"
            opacity: 0.8
        }
    }

    Item {
        y: 100; width: 150; height: 50
        Image {
            id: img3
            width: parent.width
            objectName: "item3"
            source: selectedItem == objectName ? "widebtn2.png" : "widebtn1.png"
        }
        Rectangle {
            width: parent.width + 50
            anchors.centerIn: parent
            height: img3.height - 40
            color: "red"
            opacity: 0.8
        }
    }

    onEnterFinalStage: {
        selectedItem = "item3";
        finalStageComplete = true;
    }
}
