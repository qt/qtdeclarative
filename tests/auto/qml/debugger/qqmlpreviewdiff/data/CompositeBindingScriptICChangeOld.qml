// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Old version: IC with a constant and a script binding.
import QtQuick

Item {
    property int rootMarker: 1
    component OldInner : Item {
        property int value: 3
        property int doubled: value * 2
    }
    OldInner {
        objectName: "child"
    }
}
