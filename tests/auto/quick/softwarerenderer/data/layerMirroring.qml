// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-120206
Item {
    width: 100; height: 40
    property int mirroring: ShaderEffectSource.MirrorVertically

    component Quadrants: Item {
        width: 40; height: 40
        Rectangle { width: 20; height: 20; color: "red" }
        Rectangle { x: 20; width: 20; height: 20; color: "lime" }
        Rectangle { y: 20; width: 20; height: 20; color: "blue" }
        Rectangle { x: 20; y: 20; width: 20; height: 20; color: "yellow" }
    }

    Quadrants {
        layer.enabled: true
        layer.textureMirroring: parent.mirroring
    }
    Quadrants { id: hidden; visible: false }
    ShaderEffectSource {
        x: 40; width: 40; height: 40
        sourceItem: hidden
        textureMirroring: parent.mirroring
    }
    ShaderEffectSource {
        x: 80; width: 20; height: 20
        sourceItem: hidden
        sourceRect: Qt.rect(20, 20, 20, 20)
        textureMirroring: parent.mirroring
    }
}
