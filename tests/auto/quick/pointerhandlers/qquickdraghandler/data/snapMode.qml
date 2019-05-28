/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

import QtQuick 2.12

Item {
    id: root
    objectName: "snapMode"
    width: 640
    height: 480

    Rectangle {
        id: rect1
        objectName: "rect1"
        width: 90
        height: 100
        x: 100
        y: 100
        color: "teal"

        Rectangle {
            width: parent.width/2
            height: parent.width/2
            x: width/2
            y: -x
            color: dragHandler1.active ? "red" : "salmon"

            DragHandler {
                id: dragHandler1
                objectName: "dragHandler1"
                target: rect1
            }
        }
    }


    Rectangle {
        id: rect2
        objectName: "rect2"
        width: 90
        height: 100
        x: 200
        y: 100
        color: "teal"

        DragHandler {
            id: dragHandler2
            objectName: "dragHandler2"
            target: rect2b
        }

        Rectangle {
            id: rect2b
            width: parent.width/2
            height: parent.width/2
            anchors.horizontalCenter: parent.horizontalCenter
            y: -width/2
            color: dragHandler2.active ? "red" : "salmon"
        }
    }
}
