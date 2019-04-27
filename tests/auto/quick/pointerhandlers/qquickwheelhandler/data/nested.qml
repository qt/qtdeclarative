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

import QtQuick 2.14

Rectangle {
    width: 320; height: 240
    color: "lightsteelblue"; antialiasing: true
    border.color: outerWheelHandler.active ? "red" : "white"

    WheelHandler {
        id: outerWheelHandler
        objectName: "outerWheelHandler"
        property: "x"
    }

    Rectangle {
        width: 120; height: 120; x: 100; y: 60
        color: "beige"; antialiasing: true
        border.color: innerWheelHandler.active ? "red" : "white"

        WheelHandler {
            id: innerWheelHandler
            objectName: "innerWheelHandler"
            // TODO should ideally deactivate because events go to the outer handler, not because of timeout
            activeTimeout: 0.5
            property: "x"
        }
    }
}
