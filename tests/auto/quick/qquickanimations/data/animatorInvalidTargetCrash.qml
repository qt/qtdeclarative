// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.3
import QtQuick.Window 2.2

Window {
    visible: true
    width: 100
    height: 100

    OpacityAnimator {
        id: anim
        from: 1
        to:0
        duration: 5000
        running: false
    }

    Loader {
        id: loader
        sourceComponent: Text {
            text: "Hello World"
            anchors.centerIn: parent
        }
    }

    Component.onCompleted: {
        anim.target = loader.item;
        anim.start();
        loader.active = false;
    }
}
