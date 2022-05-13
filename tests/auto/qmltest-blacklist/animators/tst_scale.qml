// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtTest 1.1

Item {
    id: root;
    width: 200
    height: 200

    TestCase {
        id: testCase
        name: "animators-scale"
        when: box.scale == 2;
        function test_endresult() {
            compare(box.scaleChangeCounter, 1);
            var image = grabImage(root);
            verify(image.pixel(0, 0) == Qt.rgba(1, 0, 0));
        }
    }

    Box {
        id: box

        ScaleAnimator {
            id: animation
            target: box
            from: 1;
            to: 2.0
            duration: 100
            easing.type: Easing.InOutCubic
            running: true
        }
    }
}
