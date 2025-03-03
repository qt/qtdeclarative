// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest

Item {
    id: root

    TestActivityCommunicator {
        id: communicator
    }

    SignalSpy {
        id: qtQuickViewStatusSpy
        target: communicator
        signalName: "qtQuickViewStatusChanged"
    }

    TestCase {
        id: validQmlFile
        name: "Valid QML file"

        function test_valid() {
            qtQuickViewStatusSpy.clear()
            communicator.loadQtQuickView("qrc:/TestViewModule/Valid.qml");
            qtQuickViewStatusSpy.wait()
            compare(qtQuickViewStatusSpy.count, 1)
            compare(qtQuickViewStatusSpy.signalArguments[0][0], 1)
        }
    }

    TestCase {
        id: invalidQmlFile
        name: "Invalid QML file"

        function test_invalid() {
            qtQuickViewStatusSpy.clear()
            communicator.loadQtQuickView("qrc:/TestViewModule/Invalid.qml");
            qtQuickViewStatusSpy.wait()
            compare(qtQuickViewStatusSpy.count, 1)
            compare(qtQuickViewStatusSpy.signalArguments[0][0], 3)
        }
    }
}
