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
        name: "animators-stopped"
        when: false
        function test_endresult() {
            verify(true);
        }
    }

    ShaderEffect {
        id: shaderEffect
        property real t;
        width: 10
        height: 10

        fragmentShader: "
            highp uniform float t;
            void main() {
                gl_FragColor = vec4(t, t, t, 1.0);
            }
        "
        UniformAnimator { id: uniformAnimator; target: shaderEffect; uniform: "t"; loops: Animation.Infinite; running: true; }
    }

    Box {
        id: box

        ScaleAnimator {    id: scaleAnimator;    target: box; loops: Animation.Infinite; running: true; }
        XAnimator {        id: xAnimator;        target: box; loops: Animation.Infinite; running: true; }
        YAnimator {        id: yAnimator;        target: box; loops: Animation.Infinite; running: true; }
        RotationAnimator { id: rotationAnimator; target: box; loops: Animation.Infinite; running: true; }
        OpacityAnimator {  id: opacityAnimator;  target: box; loops: Animation.Infinite; running: true; }

        Timer {
            id: timer;
            interval: 500
            running: true
            repeat: false
            onTriggered: {
                xAnimator.stop();
                yAnimator.stop();
                scaleAnimator.stop()
                rotationAnimator.stop();
                rotationAnimator.stop();
                uniformAnimator.stop();
                testcase.when = true;
            }
        }
    }
}
