// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma NoValue
pragma OneValue: Value
pragma MultipleValues: V1, V2, V3

import QtQml

QtObject {
    property int bad: {
        if (baz.x)
            return 1;

        if (baz.y) {
            return 2;
        }
        return 0;
    }

    onYep: {
        if (true)
            1;
        else
            1;

        for (let i in is)
            1;

        while (true)
            1;
    }
}
