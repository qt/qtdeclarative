// Copyright (C) 2017 The Qt Company Ltd.
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
            addExample("All at once", "Uses all ImageParticle features",  Qt.resolvedUrl("allatonce.qml"));
            addExample("Colored", "Colorized image particles",  Qt.resolvedUrl("colored.qml"));
            addExample("Color Table", "Color-over-life rainbow particles",  Qt.resolvedUrl("colortable.qml"));
            addExample("Deformation", "Deformed particles",  Qt.resolvedUrl("deformation.qml"));
            addExample("Rotation", "Rotated particles",  Qt.resolvedUrl("rotation.qml"));
            addExample("Sharing", "Multiple ImageParticles on the same particles",  Qt.resolvedUrl("sharing.qml"));
            addExample("Sprites", "Particles rendered with sprites",  Qt.resolvedUrl("sprites.qml"));
        }
    }
}
