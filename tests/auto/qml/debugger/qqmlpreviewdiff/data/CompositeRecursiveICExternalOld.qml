// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Old version: child is a plain Item.
import QtQuick

Item {
    property int rootMarker: 1
    Item {}
    Item {
        objectName: "child"
    }
}
