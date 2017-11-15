/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the manual tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
        objectName: "LauncherList"
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
            addExample("multibuttons", "TapHandler: gesturePolicy (99 red balloons)", Qt.resolvedUrl("multibuttons.qml"))
            addExample("flickable with Handlers", "Flickable with buttons, sliders etc. implemented in various ways", Qt.resolvedUrl("flickableWithHandlers.qml"))
        }
    }
}
