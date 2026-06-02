// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Parent component that attaches a signal handler to its child using
// grouped property syntax, like ChoosingCoffee.qml in the coffee demo.

import QtQuick

Item {
    id: wrapper
    property int callCount: 0
    property alias target: innerTarget

    SignalTargetOld {
        id: innerTarget
        onFired: callCount++
    }

    target.onFired: ++callCount
}
