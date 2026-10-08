// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    width: 200
    height: 200

    Rectangle {
        objectName: "content"
        width: 200
        height: 200
        color: "white"
        // A clip node is a batch root, so hiding and showing items below it
        // does not need a full rebuild of the batches
        clip: true

        Item {
            objectName: "outer"
            width: 200
            height: 200

            Rectangle {
                id: target
                objectName: "target"
                x: 20
                y: 20
                width: 60
                height: 60
                color: "red"
            }

            Rectangle {
                objectName: "cover"
                x: 60
                y: 60
                width: 60
                height: 60
                color: "blue"
            }
        }

        // Without an effect referencing it, an invisible item has no node in the scene
        ShaderEffectSource {
            width: 0
            height: 0
            sourceItem: target
        }

        ShaderEffectSource {
            objectName: "effectSource"
            width: 0
            height: 0
        }

        ShaderEffectSource {
            objectName: "effectSource2"
            width: 0
            height: 0
        }
    }
}
