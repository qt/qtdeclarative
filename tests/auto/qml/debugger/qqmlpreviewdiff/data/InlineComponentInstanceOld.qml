// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Inline component with an instantiated instance.
// Old version: Inner.value is 10.
import QtQuick

Item {
    component Inner : Item {
        property int value: 10
    }
    Inner {
        objectName: "inner"
    }
    property int rootMarker: 1
}
