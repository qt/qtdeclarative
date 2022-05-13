// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls
import QtQuick.Controls.impl

TestCase {
    id: testCase
    name: "Color"

    function test_transparent() {
        compare(Color.transparent("red", 0.2), Qt.rgba(1, 0, 0, 0.2))
        compare(Color.transparent(Qt.rgba(0, 1, 0, 1), 0.2), Qt.rgba(0, 1, 0, 0.2))
        compare(Color.transparent("#0000ff", 0.2), Qt.rgba(0, 0, 1, 0.2))
    }
}
