// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
Item {
    function myMethodNoArgs() { return 3; }
    function myMethod(a) { return a + 9; }
    function myMethodIndirect() { myMethod(3); }
}
