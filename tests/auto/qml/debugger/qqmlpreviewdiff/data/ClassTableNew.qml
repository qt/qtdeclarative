// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
Item {
    function test() {
        class Foo {
            greet() { return "hi" }
            static create() { return new Foo() }
        }
        return Foo.create().greet()
    }
}
