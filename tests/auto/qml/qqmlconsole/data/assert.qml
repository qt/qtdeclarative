// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

QtObject {
    property int q: 1

    function assertFail() {
        console.assert(0, "This will fail too");
    }

    Component.onCompleted: {
        const x = 12;
        console.assert(x == 12, "This will pass");
        try {
            console.assert(x < 12, "This will fail");
        } catch (e) {
            console.log(e);
        }
        console.assert("x < 12", "This will pass too");
        assertFail();
        console.assert(1);
    }
}
