// Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.4

Item {
    id: root

    property alias source: image.source
    property bool shaderActive: false

    implicitWidth: image.width

    Image {
        id: image
        objectName: "image"
        anchors { top: parent.top; bottom: parent.bottom }
        sourceSize.height: height

        visible: !shaderActive
    }

    ShaderEffect {
        id: colorizedImage

        anchors.fill: parent
        visible: shaderActive && image.status == Image.Ready
        supportsAtlasTextures: true

        property Image source: visible ? image : null

        fragmentShader: "qrc:/data/red.frag"
    }
}
