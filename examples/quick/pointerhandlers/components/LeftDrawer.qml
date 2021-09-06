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
    objectName: "LeftHandlerDrawer"
    width: 100
    height: 400
    property real stickout: 4
    property real xOpen: rect.radius * -2
    property real xClosed: stickout - width
    x: xClosed
    y: 10

    function close() {
        openCloseAnimation.to = xClosed
        openCloseAnimation.start()
    }
    function open() {
        openCloseAnimation.to = xOpen
        openCloseAnimation.start()
    }

    DragHandler {
        id: dragHandler
        yAxis.enabled: false
        xAxis.minimum: -1000
        margin: 20
        onActiveChanged:
            if (!active) {
                if (xbr.returnToBounds())
                    return;
                if (activeTranslation.x > 0)
                    open()
                if (activeTranslation.x < 0)
                    close()
            }
    }

    BoundaryRule on x {
        id: xbr
        minimum: xClosed
        maximum: xOpen
        minimumOvershoot: rect.radius
        maximumOvershoot: rect.radius
    }

    NumberAnimation on x {
        id: openCloseAnimation
        duration: 300
        easing { type: Easing.OutBounce; overshoot: 5 }
    }

    // TODO this was supposed to be RectangularGlow but we lost QtGraphicalEffects in Qt 6
    Rectangle {
        id: effect
        anchors.fill: parent
        border.width: dragHandler.margin
        border.color: "cyan"
        opacity: 0.2
        radius: rect.radius + dragHandler.margin
    }

    Rectangle {
        id: rect
        anchors.fill: parent
        anchors.margins: 3
        color: "#333"
        border.color: "cyan"
        border.width: 2
        radius: 10
        antialiasing: true
    }
}
