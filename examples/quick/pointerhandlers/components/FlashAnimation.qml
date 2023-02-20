// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

SequentialAnimation {
    id: tapFlash
    running: false
    loops: 3
    PropertyAction { value: false }
    PauseAnimation { duration: 100 }
    PropertyAction { value: true }
    PauseAnimation { duration: 100 }
}
