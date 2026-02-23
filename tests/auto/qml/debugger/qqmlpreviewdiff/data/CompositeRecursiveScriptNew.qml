// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: child is an ExternalParentScript instance (which inherits
// ExternalGrandparentScript). Tests that script bindings at both grandparent
// and parent levels are evaluated and remain reactive.
import QtQuick

Item {
    property int rootMarker: 1
    ExternalParentScript {
        objectName: "child"
    }
}
