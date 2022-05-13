// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared

Item {
    height: 480
    width: 320
    LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Dynamic Comparison", "Compares with dynamically created elements",  Qt.resolvedUrl("dynamiccomparison.qml"));
            addExample("StartStop", "Start and stop the simulation", Qt.resolvedUrl("startstop.qml"));
            addExample("Timed group changes", "Emit into managed groups",  Qt.resolvedUrl("timedgroupchanges.qml"));
            addExample("Dynamic Emitters", "Dynamically instantiated emitters with a single system",  Qt.resolvedUrl("dynamicemitters.qml"));
            addExample("Multiple Painters", "Several ParticlePainters on the same logical particles",  Qt.resolvedUrl("multiplepainters.qml"));
        }
    }
}
