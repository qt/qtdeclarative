// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick.Window 2.2
import QtQuick 2.6
import QtQml.Models 2.11
import example 1.0

Window {
    visible: true
    property bool running: rebuildTimer.running
    ListView {
        anchors.fill: parent
        model: delegateModel
    }

    DelegateModel {
        id: delegateModel
        model: objectsProvider.objects
        delegate: Item {}
    }

    Timer {
        id: rebuildTimer
        running: true
        repeat: true
        interval: 1

        property int count: 0
        onTriggered: {
            objectsProvider.rebuild();
            if (++count === 10)
                running = false;
        }
    }

    ObjectsProvider {
        id: objectsProvider
    }
}
