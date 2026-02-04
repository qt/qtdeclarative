// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    id: root
    color: "black"
    width: 320
    height: 320

    property bool mirrorImage: false

    ShaderEffectSource {
        id: doomed
        anchors.fill: parent

        sourceItem: Image {
            source: "star.png"
            visible: false
            mirror: root.mirrorImage
        }
    }
}
