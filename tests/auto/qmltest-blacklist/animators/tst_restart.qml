// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2
import QtTest 1.1

Item {
    id: root;
    width: 200
    height: 200

    property int restartCount: 5;

    TestCase {
        id: testcase
        name: "animators-restart"
        when: root.restartCount == 0 && animation.running == false;
        function test_endresult() {
            compare(box.scale, 2);
        }
    }

    Box {
        id: box

        ScaleAnimator {
            id: animation
            target: box;
            from: 1;
            to: 2.0;
            duration: 100;
            loops: 1
            running: false;
        }

        Timer {
            id: timer;
            interval: 500
            running: true
            repeat: true
            onTriggered: {
                animation.running = true;
                --root.restartCount;
            }
        }
    }
}
