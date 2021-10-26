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
import Qt.labs.animation

Item {
    id: root
    objectName: "viewport"
    default property alias data: __contentItem.data
    property alias velocity: anim.velocity
    property alias contentX: __contentItem.x // sign is reversed compared to Flickable.contentX
    property alias contentY: __contentItem.y // sign is reversed compared to Flickable.contentY
    property alias contentWidth: __contentItem.width
    property alias contentHeight: __contentItem.height
    signal flickStarted
    signal flickEnded

    Item {
        id: __contentItem
        objectName: "__contentItem"
        width: childrenRect.width
        height: childrenRect.height

        BoundaryRule on x {
            id: xbr
            minimum: root.width - __contentItem.width
            maximum: 0
            minimumOvershoot: 100
            maximumOvershoot: 100
            overshootFilter: BoundaryRule.Peak
        }

        BoundaryRule on y {
            id: ybr
            minimum: root.height - __contentItem.height
            maximum: 0
            minimumOvershoot: 100
            maximumOvershoot: 100
            overshootFilter: BoundaryRule.Peak
        }

        DragHandler {
            id: dragHandler
            onActiveChanged:
                if (active) {
                    anim.stop()
                    root.flickStarted()
                } else {
                    var vel = centroid.velocity
                    if (xbr.returnToBounds())
                        vel.x = 0
                    if (ybr.returnToBounds())
                        vel.y = 0
                    if (vel.x !== 0 || vel.y !== 0)
                        anim.restart(vel)
                    else
                        root.flickEnded()
                }
        }
        WheelHandler {
            rotationScale: 15
            property: "x"
            orientation: Qt.Horizontal
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onActiveChanged:
                // emitting signals in both instances is redundant but hard to avoid
                // when the touchpad is flicking along both axes
                if (active) {
                    anim.stop()
                    root.flickStarted()
                } else {
                    xbr.returnToBounds()
                    root.flickEnded()
                }
        }
        WheelHandler {
            rotationScale: 15
            property: "y"
            orientation: Qt.Vertical
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onActiveChanged:
                if (active) {
                    anim.stop()
                    root.flickStarted()
                } else {
                    ybr.returnToBounds()
                    root.flickEnded()
                }
        }
        MomentumAnimation {
            id: anim
            target: __contentItem
            onStarted: root.flickStarted()
            onStopped: {
                xbr.returnToBounds()
                ybr.returnToBounds()
                root.flickEnded()
            }
        }
    }
}
