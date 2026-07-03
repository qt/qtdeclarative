// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A derived type whose instances live in this (un-reloaded) CU. Each binds both
// base properties. When BaseWidget drops "gone" on reload, the "gone" bindings
// here can no longer resolve and must be dropped cleanly.

pragma ComponentBehavior: Bound

import QtQuick

Item {
    component Derived: BaseWidget {
        gone: 42
        stay: 7
    }

    Derived { objectName: "r1" }
    Derived { objectName: "r2" }
}
