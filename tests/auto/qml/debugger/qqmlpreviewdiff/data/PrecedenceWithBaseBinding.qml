// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

QtObject {
    component Base: QtObject {
        property int a: 1
    }
    component Derived: Base {
        a: 2
    }
    property QtObject der: Derived {}
    property QtObject bas: Base {}
}
