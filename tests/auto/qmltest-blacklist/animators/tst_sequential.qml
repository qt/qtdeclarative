// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtTest 1.1

Item {
    id: root;
    width: 200
    height: 200

    TestCase {
        id: testcase
        name: "animators-parallel"
        when: !animation.running
        function test_endresult() {
            compare(box.rotationChangeCounter, 1);
            compare(box.scaleChangeCounter, 1);
            compare(box.scale, 2);
            compare(box.rotation, 180);
            var image = grabImage(root);
            compare(image.pixel(0, 0), Qt.rgba(0, 0, 1, 1));
            compare(image.pixel(199, 199), Qt.rgba(1, 0, 0, 1));
        }
    }

    Box {
        id: box
        SequentialAnimation {
            id: animation
            ScaleAnimator { target: box; from: 1; to: 2.0; duration: 100; }
            RotationAnimator { target: box; from: 0; to: 180; duration: 100; }
            running: true
        }
    }
}
