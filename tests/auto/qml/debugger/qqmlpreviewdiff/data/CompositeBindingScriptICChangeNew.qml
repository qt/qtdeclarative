// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: IC changed — different constant value and script binding formula.
import QtQuick

Item {
    property int rootMarker: 1
    component NewInner : Item {
        property int value: 5
        property int tripled: value * 3
    }
    NewInner {
        objectName: "child"
    }
}
