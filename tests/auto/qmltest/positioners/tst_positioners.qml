// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.9
import QtTest 1.1

Item {
    Column {
        id: column
        Repeater {
            id: repeater
            model: 2
            Rectangle {
                width: 100
                height: 30
                border.width: 1
            }
        }
    }

    SignalSpy {
        id: spy
        target: column
        signalName: "positioningComplete"
    }

    TestCase {
        function test_forceLayout() {
            compare(column.height, 60)
            repeater.model = 4
            column.forceLayout()
            compare(column.height, 120)

            // initial positioning and our forced layout
            compare(spy.count, 2)
        }
    }
}
