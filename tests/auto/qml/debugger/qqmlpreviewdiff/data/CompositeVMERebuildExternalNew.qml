// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: child becomes an instance of an external composite type.
// The instance itself adds no properties — its VME comes from ExternalComposite.qml's root.
import QtQuick

Item {
    property int rootMarker: 1
    ExternalComposite {
        objectName: "child"
    }
}
