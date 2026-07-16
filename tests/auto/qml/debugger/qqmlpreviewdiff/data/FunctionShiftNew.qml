// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

QtObject {
    property int base: 1
    property int doubled: base * 2
    property string marker: ""
    onBaseChanged: marker = "base"
    function first() { return [1, 2].map(x => x * 3).length }
    function second() { return base + 100 }
}
