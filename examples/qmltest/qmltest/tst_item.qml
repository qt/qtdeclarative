// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.4
import QtTest 1.1

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
