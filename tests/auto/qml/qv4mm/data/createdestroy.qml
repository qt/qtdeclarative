// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml 2.2

QtObject {
    property int creations: 0
    property int destructions: 0
    property int iterations: 0

    property Component itemComponent: Component {
        QtObject {
            property var parent;
            Component.onCompleted: ++parent.creations
            Component.onDestruction: ++parent.destructions
        }
    }

    property QtObject item: null;
    property Timer timer: Timer {
        running: true
        repeat: true
        interval: 1
        onTriggered: {
            if (parent.iterations === 100) {
                item = null;
                running = false;
            } else {
                ++parent.iterations;
                item = itemComponent.createObject(null, { parent : parent });
            }
            gc();
        }
    }
}
