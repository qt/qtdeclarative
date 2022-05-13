// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtTest 1.1

Item {
    id: root

    Rectangle {
        id: box
        color: "red"
    }

    ShaderEffectSource {
        id: theSource
    }

    TestCase {
        name: "shadersource-something-to-null"
        function test_null() {
            theSource.sourceItem = box
            theSource.sourceItem = null
            verify(true); // that we got here without problems...
        }
    }
}
