// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {

    component Patron: QtObject {
        property int foo
        Component.onCompleted: console.log(foo)
    }

    component Mafik: Patron {
        property int bar: foo
    }

    property int foo // should not be in inlineUsages
    property var realFoo: Mafik {
        function f() {
            return foo; // should be in inlineUsages
        }
    }

    property InlineComponentProvider fromAnotherFile: InlineComponentProvider {}
    property InlineComponentProvider.IC1 fromAnotherFile2: InlineComponentProvider.IC1 {}
}
