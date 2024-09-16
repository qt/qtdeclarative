// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import Qt.test 1.0

TestObject {
    function myCustomFunction() {
        return 0;
    }

    onMySignal: { for (var ii = 0; ii < 10000; ++ii) { myCustomFunction(); } }
}
