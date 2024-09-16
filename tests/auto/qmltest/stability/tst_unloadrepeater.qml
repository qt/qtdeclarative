// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2
import QtTest 1.1

Item {
    width : 800
    height : 600

    Timer {
        id: probablyOkNow
        interval: 2000
        running: true
        repeat: false
        onTriggered: testCase.when = true;
    }

    TestCase {
        id: testCase
        name: "unloaded-repeater"
        when: false
        function test_endresult()
        {
            havocTimer.running = false;
            verify(true, "If we didn't crash by now, all is good")
        }
    }

    Timer {
        id: havocTimer
        interval: 1
        running: true
        repeat: true

        onTriggered: {
            loader.sourceComponent =  null
            loader.sourceComponent = component1
        }

    }

    Loader {
        id : loader
        asynchronous : true
    }

    Component {
        id : component1
        Grid {
            columns: 70
            spacing: 2

            Repeater {
                model : 2000

                Rectangle {
                    width : 3
                    height : 3
                    color : "blue"
                }
            }
        }
    }
}
