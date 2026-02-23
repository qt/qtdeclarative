// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Old version: child is a plain Item with no inline component.
// Uses a filler Item so the child is at the same object index (2)
// as the IC instance in the new version (IC root goes to index 1).
import QtQuick

Item {
    property int rootMarker: 1
    Item { width: 10 }
    Item {
        objectName: "child"
    }
}
