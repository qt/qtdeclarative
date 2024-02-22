// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import Qt.test 1.0

TestObject {
    function myCustomFunction(b) {
        var n = b;
        var a = 1;
        while (n > 0) {
            a = a * n;
            n--;
        }
        return a;
    }

    onMySignal: { for (var ii = 0; ii < 10000; ++ii) { myCustomFunction(10); } }
}

