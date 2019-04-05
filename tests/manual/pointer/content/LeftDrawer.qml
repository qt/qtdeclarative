/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
import QtQuick 2.14
import Qt.labs.animation 1.0
import QtGraphicalEffects 1.14

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
                if (translation.x > 0)
                    open()
                if (translation.x < 0)
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

    RectangularGlow {
        id: effect
        anchors.fill: parent
        glowRadius: dragHandler.margin
        spread: 0.2
        color: "cyan"
        cornerRadius: rect.radius + glowRadius
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
