// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2
import QtTest 1.1

Item {
    id: root;
    width: 200
    height: 200

    TestCase {
        id: testCase
        name: "animators-on"
        when: !animx.running && !animy.running
              && !anims.running && !animr.running
              && !animo.running;
        function test_endresult() {
            tryCompare(box, 'xChangeCounter', 1);
            compare(box.yChangeCounter, 1);
            compare(box.scaleChangeCounter, 1);
            compare(box.rotationChangeCounter, 1);
            compare(box.opacityChangeCounter, 1);
            compare(box.x, 100);
            compare(box.y, 100);
            compare(box.scale, 2);
            compare(box.rotation, 180);
            compare(box.opacity, 0.5);
        }
    }

    Box {
        id: box
        anchors.centerIn: undefined
        XAnimator on x { id: animx; from: 0; to: 100; duration: 100 }
        YAnimator on y { id: animy; from: 0; to: 100; duration: 100 }
        ScaleAnimator on scale { id: anims; from: 1; to: 2; duration: 100 }
        RotationAnimator on rotation { id: animr ; from: 0; to: 180; duration: 100 }
        OpacityAnimator on opacity { id: animo; from: 1; to: 0.5; duration: 100 }
    }
}
