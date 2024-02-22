// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import Qt.labs.platform

TestCase {
    id: testCase
    width: 200
    height: 200
    name: "MenuSeparator"

    Component {
        id: menuSeparator
        MenuSeparator { }
    }

    function test_separator() {
        var separator = menuSeparator.createObject(testCase)
        verify(separator)

        compare(separator.separator, true)

        separator.destroy()
    }
}
