// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: child becomes an instance of an external composite with script binding.
import QtQuick

Item {
    property int rootMarker: 1
    ExternalCompositeScript {
        objectName: "child"
    }
}
