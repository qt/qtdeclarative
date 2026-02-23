// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQml

QtObject {
    id: root
    property int base: 5
    property int derived: root.base + 1
    property bool newFlag: true   // PropertyAdded forces Reattach
}
