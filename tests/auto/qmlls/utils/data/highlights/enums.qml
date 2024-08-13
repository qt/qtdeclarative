// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
import "./" as I

QtObject {
    enum Osc {
        Sin,
        Saw = 1
    }

    property int i: I.Identifiers.K.Plus
}
