// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

SequentialAnimation {
    id: tapFlash
    running: false
    PropertyAction { value: false }
    PauseAnimation { duration: 100 }
    PropertyAction { value: true }
    PauseAnimation { duration: 100 }
    PropertyAction { value: false }
    PauseAnimation { duration: 100 }
    PropertyAction { value: true }
    PauseAnimation { duration: 100 }
    PropertyAction { value: false }
    PauseAnimation { duration: 100 }
    PropertyAction { value: true }
}
