/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
import Qt.labs.handlers 1.0

Item {
    id: root
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

        property real xlimit: root.width - __contentItem.width
        property real ylimit: root.height - __contentItem.height

        function returnToBounds() {
            if (x > 0)
                x = 0
            else if (x < xlimit)
                x = xlimit
            if (y > 0)
                y = 0
            else if (y < ylimit)
                y = ylimit
        }

        DragHandler {
            id: dragHandler
            onActiveChanged: if (!active) anim.restart(velocity)
        }
        MomentumAnimation {
            id: anim
            target: __contentItem
            onStarted: root.flickStarted()
            onStopped: {
                __contentItem.returnToBounds()
                root.flickEnded()
            }
        }
    }
}
