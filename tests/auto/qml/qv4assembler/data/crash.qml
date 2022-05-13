// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml 2.2
import Crash 1.0

QtObject {
    property Crash crash: Crash {
        id: crash
    }

    // Recursion makes the crash more reliable
    // With a single frame the unwinder might guess
    // the next frame by chance.
    function recurse(x) {
        if (x > 32)
            crash.crash();
        else
            recurse(x + 1);
    }

    property Timer timer: Timer {
        interval: 10
        running: true
        onTriggered: recurse(0)
    }
}

