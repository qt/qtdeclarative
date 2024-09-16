// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtTest 1.1

Item {
    Rectangle {
        id: rect

        property bool finished: false

        Behavior on opacity {
            NumberAnimation {
                onRunningChanged: {
                    if (!running) {
                        if (rect.opacity <= 0.1)
                            rect.opacity = 1
                        else
                            rect.finished = true
                    }
                }
            }
        }
    }

    TestCase {
        name: "AbstractAnitaionJobCrash"

        function test_noCrash() {
            rect.opacity = 0
            while (!rect.finished)
                wait(100)
        }
    }
}
