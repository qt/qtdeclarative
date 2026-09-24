// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root
    width: 100
    height: 100
    scale: 1

    // Lazy.qml, and with it Constants.mjs, is only requested once the test activates the loader
    // via an in-place update.
    Loader {
        id: loader
        active: false
        source: "Lazy.qml"
    }

    Timer {
        repeat: true
        interval: 30
        running: true
        onTriggered: {
            console.log("lazy_module_test scale=" + root.scale + " answer="
                        + (loader.item ? loader.item.answer : "none"));
        }
    }
}
