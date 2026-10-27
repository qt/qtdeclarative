// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Instantiates Base directly. The instances add no properties, signals or functions of their own,
// so they need no VME meta-object and share Base's property cache in this unit. The root captures
// the alias on them in its bindings.

import QtQuick

Item {
    Base { id: b1; objectName: "b1" }
    Base { id: b2; objectName: "b2" }

    property int seen1: b1.valueAlias
    property int seen2: b2.valueAlias
}
