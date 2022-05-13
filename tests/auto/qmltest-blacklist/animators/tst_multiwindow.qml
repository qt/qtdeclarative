// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtTest 1.1
import QtQuick.Window 2.0

Item {
    id: root;
    width: 200
    height: 200

    TestCase {
        id: testCase
        name: "animators-mixed"
        when: countdown == 0
        function test_endresult() {
            verify(true, "Just making sure we didn't crash");
        }
    }

    property int countdown: 5;

    Window {
        id: window

        width: 100
        height: 100

        ShaderEffect {
            width: 50
            height: 50

            property real t;
            UniformAnimator on t { from: 0; to: 1; duration: 1000; loops: Animation.Infinite }
            RotationAnimator on rotation { from: 0; to: 360; duration: 1000; loops: Animation.Infinite }
            ScaleAnimator on scale { from: 0.5; to: 1.5; duration: 1000; loops: Animation.Infinite }
            XAnimator on x { from: 0; to: 50; duration: 1000; loops: Animation.Infinite }
            YAnimator on y { from: 0; to: 50; duration: 1000; loops: Animation.Infinite }
            OpacityAnimator on opacity { from: 1; to: 0.5; duration: 1000; loops: Animation.Infinite }

            fragmentShader: "
                uniform lowp float t;
                uniform lowp float qt_Opacity;
                varying highp vec2 qt_TexCoord0;
                void main() {
                    gl_FragColor = vec4(qt_TexCoord0, t, 1) * qt_Opacity;
                }
                "
        }

        visible: true
    }

    Timer {
        interval: 250
        running: true
        repeat: true
        onTriggered: {
            if (window.visible)
                --countdown
            window.visible = !window.visible;
        }
    }
}
