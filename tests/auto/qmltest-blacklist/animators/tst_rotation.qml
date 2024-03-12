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
        name: "animators-rotation"
        when: box.rotation == 180
        function test_endresult() {
            compare(box.rotationChangeCounter, 1);
            var image = grabImage(root);
            verify(image.pixel(50, 50) == Qt.rgba(0, 0, 1));
        }
    }

    Box {
        id: box
        RotationAnimator {
            id: animation
            target: box
            from: 0;
            to: 180
            duration: 100
            easing.type: Easing.InOutBack
            running: true
        }
    }
}
