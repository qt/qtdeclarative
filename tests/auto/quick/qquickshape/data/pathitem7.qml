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
import tst_qquickpathitem 1.0

Item {
    id: item
    width: 200
    height: 150

    Shape {
        vendorExtensionsEnabled: false
        objectName: "shape"
        id: shape
        anchors.fill: parent

        ShapePath {
            strokeWidth: 4
            strokeColor: "red"
            scale: Qt.size(shape.width - 1, shape.height - 1)
            fillGradient: LinearGradient {
                x1: 20; y1: 20
                x2: 180; y2: 130
                GradientStop { position: 0; color: "blue" }
                GradientStop { position: 0.2; color: "green" }
                GradientStop { position: 0.4; color: "red" }
                GradientStop { position: 0.6; color: "yellow" }
                GradientStop { position: 1; color: "cyan" }
            }
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 4 ]
            startX: 20; startY: 20 // unnecessary, PathPolyline moves to the first vertex.
            PathPolyline {
                path: [ Qt.point(20.0  / (item.width - 1.0), 20.0  / (item.height - 1.0)),
                        Qt.point(180.0 / (item.width - 1.0), 130.0 / (item.height - 1.0)),
                        Qt.point(20.0  / (item.width - 1.0), 130.0 / (item.height - 1.0)),
                        Qt.point(20.0  / (item.width - 1.0), 20.0  / (item.height - 1.0)) ]
            }
        }
    }
}
