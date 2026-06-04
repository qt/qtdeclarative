// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Intermediate layer: instantiates the form and aliases both the form instance
// and the list property so the wrapper can access individual timers.

import QtQuick

Item {
    id: outerRoot
    property alias target: innerTarget
    property alias targetTimers: innerTarget.timers

    ListChildFormOld {
        id: innerTarget
    }
}
