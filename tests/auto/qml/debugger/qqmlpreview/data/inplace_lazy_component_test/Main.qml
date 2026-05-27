// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id: root
    width: 320
    height: 240

    // The child component is wrapped in a Component{} — not instantiated at startup.
    Component {
        id: lazyComponent
        LazyChild {}
    }

    // Signal that we are ready (child is NOT yet instantiated).
    Timer {
        id: readyTimer
        interval: 50
        running: true
        onTriggered: console.log("lazy_component_test ready")
    }

    // After a delay, instantiate the lazy component via a Loader.
    Loader {
        id: loader
        anchors.fill: parent
    }

    Timer {
        id: instantiateTimer
        interval: 500
        running: true
        onTriggered: loader.sourceComponent = lazyComponent
    }

    // Periodically report the label once the loader has an item.
    Timer {
        repeat: true
        interval: 30
        running: loader.item !== null
        onTriggered: console.log("lazy_component_test label=" + loader.item.label)
    }
}
