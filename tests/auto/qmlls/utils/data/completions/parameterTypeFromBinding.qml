// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import completions.CppTypes

Rectangle {
    onColorChanged: (col) => console.log(col.r)

    WithSignal {
        onHelloSignal: (someType) => console.log(someType.x)
    }
    WithFakePropertyChangedSignal {
        onMySomeTypeChanged: (someType) => console.log(someType.x)
    }
}
