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
        name: "animators-nested"
        when: !animation.running
        function test_endresult() {
            compare(box.before, 2);
            compare(box.after, 2);
        }
    }

    Box {
        id: box

        anchors.centerIn: undefined

        property int before: 0;
        property int after: 0;

        SequentialAnimation {
            id: animation;
            ScriptAction { script: box.before++; }
            ParallelAnimation {
                ScaleAnimator { target: box; from: 2.0; to: 1; duration: 100; }
                OpacityAnimator { target: box; from: 0; to: 1; duration: 100; }
            }
            PauseAnimation { duration: 100 }
            SequentialAnimation {
                ParallelAnimation {
                    XAnimator { target: box; from: 0; to: 100; duration: 100 }
                    RotationAnimator { target: box; from: 0; to: 90; duration: 100 }
                }
                ParallelAnimation {
                    XAnimator { target: box; from: 100; to: 0; duration: 100 }
                    RotationAnimator { target: box; from: 90; to: 0; duration: 100 }
                }
            }
            ScriptAction { script: box.after++; }
            running: true
            loops: 2
        }
    }

}
