// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Item {
    focus: true

    Loader {
        anchors.fill: parent
        id: loader
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_S)
            loader.source = "";

        if (event.key === Qt.Key_R)
            loader.source = "qrc:/SimpleRect.qml";
        if (event.key === Qt.Key_3)
            loader.source = "qrc:/Rects.qml";
        if (event.key === Qt.Key_4)
            loader.source = "qrc:/LotsOfRects.qml";
        if (event.key === Qt.Key_5)
            loader.source = "qrc:/MultiClipRects.qml";
        if (event.key === Qt.Key_I)
            loader.source = "qrc:/Images.qml";
        if (event.key === Qt.Key_A)
            loader.source = "qrc:/AtlasedImages.qml";
        if (event.key === Qt.Key_P)
            loader.source = "qrc:/Painter.qml";
        if (event.key === Qt.Key_C)
            loader.source = "qrc:/CompressedImages.qml";
        if (event.key === Qt.Key_T)
            loader.source = "qrc:/Text.qml";
        if (event.key === Qt.Key_D)
            loader.source = "qrc:/DistanceFieldText.qml";
        if (event.key === Qt.Key_L)
            loader.source = "qrc:/Layers.qml";
        if (event.key === Qt.Key_6)
            loader.source = "qrc:/ShaderEffectSource.qml";
        if (event.key === Qt.Key_E)
            loader.source = "qrc:/ShaderEffect.qml";
        if (event.key === Qt.Key_Z)
            loader.source = "qrc:/ShaderEffectNoAnim.qml";
        if (event.key === Qt.Key_G)
            helper.testGrabWindow()
        if (event.key === Qt.Key_F)
            helper.testGrabItem(loader.item)
        if (event.key === Qt.Key_W)
            loader.source = "qrc:/MoreWindows.qml";
        if (event.key === Qt.Key_N)
            loader.source = "qrc:/LotsOfNodes.qml";
    }
}
