// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQml

QtObject {
    property list<QtObject> items: [
        QtObject { property int val: 1 },
        QtObject { property int val: 2 }
    ]
    property bool newFlag: true   // PropertyAdded forces Reattach
}
