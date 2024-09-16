// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import "script.js" as Script
import "script2.js" as OtherScript

Item {
    function runtest() {
        var a = 0;
        for (var ii = 0; ii < 1000000; ++ii)
            a += Script.func();
        return a;
    }
}

