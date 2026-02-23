// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Outer wrapper that instantiates the composite type.
// Provides the outer context (the "ApplicationFlow" equivalent).

import QtQuick

Item {
    id: outerRoot
    width: 300
    height: 300

    CompositeContextIdLookupOld {
        id: derived
        width: parent.width
        height: parent.height
        smallMode: true
    }
}
