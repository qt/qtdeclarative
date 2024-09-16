// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2
import QtTest 1.1

Item {
    id: root;
    width: 200
    height: 200

    TestCase {
        id: testcase
        name: "animators-behavior"
        when: box.scale == 2
        function test_endresult() {
            compare(box.scaleChangeCounter, 1);
            compare(box.scale, 2);
            var image = grabImage(root);

            verify(image.pixel(0, 0) == Qt.rgba(1, 0, 0));
            verify(image.pixel(199, 199) == Qt.rgba(0, 0, 1));
        }
    }

    Box {
        id: box
        Behavior on scale { ScaleAnimator { id: animation; duration: 100; } }
    }

    Timer {
        interval: 100;
        repeat: false
        running: true
        onTriggered: box.scale = 2
    }
}
