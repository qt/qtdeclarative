// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//! [local]
Item {
    x: 50
    y: 100

    // prints: QRectF(-10, -20, 30, 40)
    Component.onCompleted: print(childrenRect)

    Item {
        x: -10
        y: -20
        width: 30
        height: 40
    }
}
//! [local]
