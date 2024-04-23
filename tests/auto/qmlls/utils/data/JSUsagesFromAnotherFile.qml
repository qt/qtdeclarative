// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

JSUsages {
    function f() {
        // sanity check: reuse variable names from function f in JSUsages. Those should not appear
        // as usages of the sum of JSUsages.
        let sum = 0;
        sum += 1;
        sum += helloProperty + 32 // valid usage of JSUsages's helloProperty
        return sum
    }

}
