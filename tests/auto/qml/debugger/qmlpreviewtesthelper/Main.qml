// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

Timer {
    interval: 10
    running: true
    repeate: true
    onTriggered: console.log("HELPER_COMPILED_RESOURCE")
}
