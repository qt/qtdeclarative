// Copyright (C) 2016 Gunnar Sletta <gunnar@sletta.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2
import QtTest 1.1

Item {
    id: root;
    width: 200
    height: 200

    TestCase {
        id: testcase
        name: "animators-targetdestroyed"
        when: false
        function test_endresult() {
            verify(true, "Got here :)");
        }
    }

    Rectangle {
        id: box
        width: 10
        height: 10
        color: "steelblue"
    }

    YAnimator {
        id: anim
        target: box
        from: 0;
        to: 100
        duration: 100
        loops: Animation.Infinite
        running: true
    }

    SequentialAnimation {
        running: true
        PauseAnimation { duration: 150 }
        ScriptAction { script: box.destroy(); }
        PauseAnimation { duration: 50 }
        ScriptAction { script: anim.destroy(); }
        PauseAnimation { duration: 50 }
        ScriptAction { script: testcase.when = true }
    }
}
