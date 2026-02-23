// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: IC 'Inner' derives from ExternalComposite (external composite type).
// The child instantiates Inner. Tests recursive composite: Inner's own VME
// stacks on top of ExternalComposite's VME.
import QtQuick

Item {
    property int rootMarker: 1

    component Inner: ExternalComposite {
        property int extra: 77
    }

    Inner {
        objectName: "child"
    }
}
