// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQml
import Test
import Qt.labs.synchronizer

QtObject {
    id: root

    property int count: 11
    property Unnotified unnotified: Unnotified {
        objectName: root.objectName
    }

    property string sourceProperty: "unnotified"

    Synchronizer on count {
        sourceObject: unnotified
        sourceProperty: root.sourceProperty
    }
}
