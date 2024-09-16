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
        name: "animators-opacity"
        when: box.opacity == 0.5
        function test_endresult() {
            compare(box.opacityChangeCounter, 1);
            var image = grabImage(root);
            compare(image.red(50, 50), 255);
            verify(image.green(50, 50) > 0);
            verify(image.blue(50, 50) > 0);
        }
    }

    Box {
        id: box

        OpacityAnimator {
            id: animation
            target: box
            from: 1;
            to: 0.5
            duration: 100
            running: true
        }
    }
}
