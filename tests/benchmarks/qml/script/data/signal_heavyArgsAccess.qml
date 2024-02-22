// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import Qt.test 1.0

TestObject {
    onMySignalWithArgs: {
        var a = 0;
        for (var i = 0; i < 10000; ++i)
            a += n;
        return a;
    }
}
