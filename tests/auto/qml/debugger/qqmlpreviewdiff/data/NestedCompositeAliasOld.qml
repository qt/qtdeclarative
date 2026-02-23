// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Nested composite hierarchy with aliases at each level.
// DerivedCompositeWithAliases inherits CompositeBaseWithAliases.

import QtQuick

Item {
    width: 400
    height: 400

    DerivedCompositeWithAliases {
        id: nested
        width: parent.width
        height: parent.height
        headerText: "Deep Nested"
        counter: 42
    }
}
