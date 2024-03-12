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
        name: "animators-y"
        when: box.y == 100
        function test_endresult() {
            compare(box.yChangeCounter, 1);
            var image = grabImage(root);
            verify(image.pixel(0, 100) == Qt.rgba(1, 0, 0));
            verify(image.pixel(0, 99) == Qt.rgba(1, 1, 1)); // outside on the top
        }
    }

    Box {
        id: box

        anchors.centerIn: undefined

        YAnimator {
            id: animation
            target: box
            from: 0;
            to: 100
            duration: 100
            running: true
        }
    }
}
