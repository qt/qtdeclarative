// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.4
import QtTest 1.1

Item {
    id: root
    width: 500
    height: 500

    Timer {
        id: timer
        interval: 1
        running: true
        repeat: true
        onTriggered: {
            if (myCanvas.parent == root) {
                myCanvas.parent = null
            } else {
                myCanvas.parent = root
            }
        }
    }

    Canvas {
        id: myCanvas
        anchors.fill: parent
        property var paintContext: null

        function doPaint() {
            paintContext.fillStyle = Qt.rgba(1, 0, 0, 1);
            paintContext.fillRect(0, 0, width, height);
            requestAnimationFrame(doPaint);
        }

        onAvailableChanged: {
            if (available) {
                paintContext = getContext("2d")
                requestAnimationFrame(doPaint);
            }
        }
    }

    TestCase {
        name: "invalidContext"
        when: myCanvas.parent === null && myCanvas.paintContext !== null

        function test_paintContextInvalid() {
            verify(myCanvas.paintContext);
            var caught = false;
            try {
                console.log(myCanvas.paintContext.fillStyle);
            } catch(e) {
                caught = true;
            }
            verify(caught);
            timer.running = false
        }
    }
}
