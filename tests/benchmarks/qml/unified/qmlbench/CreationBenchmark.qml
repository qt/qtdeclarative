// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QmlBench

// The base of all *creation* benchmarks, API-compatible with the qmlbench tool.
// On every tick the original destroys and recreates all `count` items; we keep
// that behaviour but tie it to the harness-driven `t` instead of an animation.
// Component.onCompleted performs the initial creation (the object-creation phase).
Benchmark {
    id: root

    property alias delegate: internalRepeater.delegate
    property alias repeater: internalRepeater

    onTChanged: {
        internalRepeater.model = 0;
        internalRepeater.model = root.count;
    }

    Component.onCompleted: internalRepeater.model = root.count

    Repeater {
        id: internalRepeater
    }
}
