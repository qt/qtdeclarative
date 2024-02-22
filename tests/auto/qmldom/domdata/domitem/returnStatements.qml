// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

QtObject {
    // TODO: Add a test for returning void
    function returningFunction(i) {
        if (i)
            return 123;
        else
            return 1 + 2;
    }
}
