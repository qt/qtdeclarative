// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root

    width: 200
    height: 200

    property bool useVertexShader: true

    function destroyContainer() {
        container.destroy()
    }

    Rectangle {
        id: content

        width: 100
        height: 100

        color: "red"
    }

    Item {
        id: container
        objectName: "container"

        anchors.fill: parent

        // Declared before the effect, so it is destroyed first.
        ShaderEffectSource {
            id: textureSource

            width: 100
            height: 100

            sourceItem: content

            visible: false
        }

        // Both the vertex and the fragment shader sample `source`.
        ShaderEffect {
            objectName: "effect"

            width: 100
            height: 100

            property Item source: textureSource
            property rect subRect: Qt.rect(0, 0, 100, 100)

            vertexShader: root.useVertexShader ? "sharedSource.vert.qsb" : ""
            fragmentShader: "sharedSource.frag.qsb"
        }
    }
}
