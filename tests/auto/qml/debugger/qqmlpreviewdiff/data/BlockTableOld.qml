// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
Item {
    function test() {
        let result = 0
        try { let a = 1; result += a } catch(e) { }
        return result
    }
}
