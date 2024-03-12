// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest

Rectangle {
    id: foo
    width: 640; height: 480
    color: "cyan"

    TestCase {
        name: "ItemTests"
        id: test1

        function test_color() {
            compare(foo.color, "#00ffff")
        }
    }
}
