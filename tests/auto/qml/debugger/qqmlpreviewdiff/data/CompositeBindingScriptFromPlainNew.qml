// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: child becomes an instance of an IC with script bindings.
// The script binding 'doubled' depends on the constant 'value'.
import QtQuick

Item {
    property int rootMarker: 1
    component Inner : Item {
        property int value: 7
        property int doubled: value * 2
    }
    Inner {
        objectName: "child"
    }
}
