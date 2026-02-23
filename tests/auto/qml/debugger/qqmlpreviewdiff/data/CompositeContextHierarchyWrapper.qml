// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Outer wrapper that instantiates the composite derived type.
// This file is NOT the one being patched — it provides the outer context
// that the derived type's objects will live in.
// Mimics ApplicationFlow.qml instantiating Home.

import QtQuick

Item {
    id: outerRoot
    width: 300
    height: 300

    CompositeContextDerivedOld {
        id: derived
        width: parent.width
        height: parent.height
        smallMode: true
    }
}
