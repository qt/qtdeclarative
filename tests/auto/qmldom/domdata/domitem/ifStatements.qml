// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQml

QtObject {
    function conditional() {
        let i = 5
        if (i)
            i = 42

        if (i == 55)
            i = 32
        else
            i = i - 1

        if (i == 42) {
            i = 111
        }

        if (i == 746) {
            i = 123
        } else {
            i = 456
        }
    }
}
