/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
