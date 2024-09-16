// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtTest 1.1

TestCase {
    name: "SelfTests_stringify"

    function test_stringify_rect() {
        var rect = Qt.rect(0, 1, 2, 3)
        var stringifiedRect = qtest_results.stringify(rect)
        verify(stringifiedRect.length > 0)
        compare(stringifiedRect, rect.toString())
    }
}

