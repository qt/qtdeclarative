// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml 2.12

QtObject {
    id: root
    property QtObject self;

    property Timer timer: Timer {
        running: true
        interval: 1
        onTriggered: {
            root.assignThis();
            root.self = null;
            root.assignThis();
        }
    }

    function getThis() {
        return this;
    }

    function assignThis() {
        self = getThis();
    }
}
