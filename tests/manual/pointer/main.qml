/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the manual tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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

import QtQuick 2.8
import QtQuick.Window 2.2
import "qrc:/quick/shared/" as Examples

Window {
    width: 800
    height: 600
    visible: true
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("single point handler", "QQuickPointerSingleHandler: test properties copied from events", Qt.resolvedUrl("singlePointHandlerProperties.qml"))
            addExample("joystick", "DragHandler: move one item inside another with any pointing device", Qt.resolvedUrl("joystick.qml"))
            addExample("mixer", "mixing console", Qt.resolvedUrl("mixer.qml"))
            addExample("pinch", "PinchHandler: scale, rotate and drag", Qt.resolvedUrl("pinchHandler.qml"))
            addExample("map", "scale and pan", Qt.resolvedUrl("map.qml"))
            addExample("custom map", "scale and pan", Qt.resolvedUrl("map2.qml"))
            addExample("fling animation", "DragHandler: after dragging, use an animation to simulate momentum", Qt.resolvedUrl("flingAnimation.qml"))
            addExample("fake Flickable", "implementation of a simplified Flickable using only Items, DragHandler and MomentumAnimation", Qt.resolvedUrl("fakeFlickable.qml"))
            addExample("photo surface", "re-implementation of the existing photo surface demo using Handlers", Qt.resolvedUrl("photosurface.qml"))
            addExample("tap", "TapHandler: device-agnostic tap/click detection for buttons", Qt.resolvedUrl("tapHandler.qml"))
        }
    }
}
