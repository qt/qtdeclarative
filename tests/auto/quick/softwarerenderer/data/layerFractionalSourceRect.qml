// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// Source rects smaller than a logical pixel
Item {
    width: 40; height: 20
    property int mirroring: ShaderEffectSource.MirrorVertically

    Item {
        id: quadrants
        width: 40; height: 40
        visible: false
        Rectangle { width: 20; height: 20; color: "red" }
        Rectangle { x: 20; y: 20; width: 20; height: 20; color: "yellow" }
    }
    ShaderEffectSource {
        width: 10; height: 10
        sourceItem: quadrants
        sourceRect: Qt.rect(0, 0, 0.5, 0.5)
        textureMirroring: parent.mirroring
    }
    ShaderEffectSource {
        x: 20; width: 10; height: 10
        sourceItem: quadrants
        sourceRect: Qt.rect(20.25, 20.25, 0.5, 0.5)
        textureMirroring: parent.mirroring
    }
}
