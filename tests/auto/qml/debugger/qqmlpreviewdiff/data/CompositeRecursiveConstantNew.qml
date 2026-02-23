// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: child is an ExternalParent instance (which inherits ExternalGrandparent).
// Tests that both grandparent and parent constant bindings are evaluated.
import QtQuick

Item {
    property int rootMarker: 1
    ExternalParent {
        objectName: "child"
    }
}
