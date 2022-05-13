// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import Qt.test 1.0

TestObject {
    onMySignalWithArgs: { var a = 1; return a; }
}

