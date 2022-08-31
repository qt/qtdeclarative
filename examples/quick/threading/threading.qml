// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared as Examples

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Threaded ListModel", "Updates a ListModel in another thread", Qt.resolvedUrl("threadedlistmodel/timedisplay.qml"));
            addExample("WorkerScript", "Performs calculations in another thread", Qt.resolvedUrl("workerscript/workerscript.qml"));
        }
    }
}
