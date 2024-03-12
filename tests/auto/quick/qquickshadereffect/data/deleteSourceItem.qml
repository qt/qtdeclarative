// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    ShaderEffect {
        id: sei
        property variant source
    }

    ShaderEffectSource {
        id: doomedses
        hideSource: true
        sourceItem: Image {
            id: doomed
            source: "star.png"
        }
    }

    function setDeletedSourceItem() {
        doomed.destroy();
        sei.source = doomedses;
        // now set a fragment shader to trigger source texture detection.
        sei.fragmentShader = "qrc:/data/test.frag";
    }
}
