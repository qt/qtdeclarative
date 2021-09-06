/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

import QtQuick
import QtQuick.Layouts
import shared as Examples
import "components"

Rectangle {
    id: window
    width: 800
    height: 800
    visible: true
    Examples.LauncherList {
        id: ll
        objectName: "LauncherList"
        anchors.fill: parent
        Component.onCompleted: {
            addExample("tap", "TapHandler: device-agnostic tap/click detection for buttons", Qt.resolvedUrl("tapHandler.qml"))
            addExample("multibuttons", "TapHandler: gesturePolicy (99 red balloons)", Qt.resolvedUrl("multibuttons.qml"))
            addExample("single point handler", "PointHandler: properties such as seat, device, modifiers, velocity, pressure", Qt.resolvedUrl("singlePointHandlerProperties.qml"))
            addExample("hover sidebar", "HoverHandler: a hierarchy of items sharing the hover state", Qt.resolvedUrl("sidebar.qml"))
            addExample("joystick", "DragHandler: move one item inside another with any pointing device", Qt.resolvedUrl("joystick.qml"))
            addExample("mixer", "DragHandler: drag multiple sliders with multiple fingers", Qt.resolvedUrl("mixer.qml"))
            addExample("fling animation", "DragHandler: after dragging, use an animation to simulate momentum", Qt.resolvedUrl("flingAnimation.qml"))
            addExample("pinch", "PinchHandler: scale, rotate and drag", Qt.resolvedUrl("pinchHandler.qml"))
            addExample("map", "scale, pan, re-render at different resolutions", Qt.resolvedUrl("map.qml"))
            addExample("fake Flickable", "implementation of a simplified Flickable using only Items, DragHandler and MomentumAnimation", Qt.resolvedUrl("fakeFlickable.qml"))
            addExample("tablet canvas", "PointHandler and HoverHandler with a tablet: detect the stylus, and draw", Qt.resolvedUrl("tabletCanvasDrawing.qml"))
        }
    }
    Item {
        id: glassPane
        objectName: "glassPane"
        z: 10000
        anchors.fill: parent

        // TODO use Instantiator to create these... but we need to be able to set their parents to glassPane somehow (QTBUG-64546)
        TouchpointFeedbackSprite { }
        TouchpointFeedbackSprite { }
        TouchpointFeedbackSprite { }
        TouchpointFeedbackSprite { }
        TouchpointFeedbackSprite { }
        TouchpointFeedbackSprite { }

        MouseFeedbackSprite { }
    }
}
