// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import Qt.test 1.0

TestObject {
    id: obj
    property real inc: 3

    onMySignalWithArgs: {
        var a = 0;
        for (var i = 0; i < 10000; ++i)
            a += obj.inc;
        return a;
    }
}
