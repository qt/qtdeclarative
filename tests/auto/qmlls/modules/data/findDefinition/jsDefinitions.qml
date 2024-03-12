// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15

Item {
    id: rootId

    property int i // (1)
    function f(a /*(2)*/ , b) {return a /*  go to definition on a leads to (2) */  > b}  // (4)

    Component.onCompleted: {
        let x = 42 // (3)
        f(x, i) // goto definition on f goes to 4, on x goes to (3) and on i goes to (1)
        f(x, rootId.i) // goto definition on f goes to 4, on x goes to (3) and on i goes to (1)
    }
}
