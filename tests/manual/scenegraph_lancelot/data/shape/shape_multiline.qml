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
import QtQuick.Shapes 1.14

Item {
    id: root
    width: 320
    height: 320

    Shape {
        vendorExtensionsEnabled: false
        anchors.fill: parent

        ShapePath {
            strokeWidth: 1
            strokeColor: "red"
            fillColor: Qt.rgba(1,0,0,0.3)
            scale: Qt.size(root.width - 1, root.height - 1)
            PathMultiline {
                paths: [
                        [Qt.point(0.5,     0.06698),
                         Qt.point(1,       0.93301),
                         Qt.point(0,       0.93301),
                         Qt.point(0.5,     0.06698)],

                        [Qt.point(0.5,     0.12472),
                         Qt.point(0.95,    0.90414),
                         Qt.point(0.05,    0.90414),
                         Qt.point(0.5,     0.12472)],

                        [Qt.point(0.47131, 0.32986),
                         Qt.point(0.36229, 0.64789),
                         Qt.point(0.51492, 0.58590),
                         Qt.point(0.47563, 0.76014),
                         Qt.point(0.44950, 0.73590),
                         Qt.point(0.46292, 0.83392),
                         Qt.point(0.52162, 0.75190),
                         Qt.point(0.48531, 0.76230),
                         Qt.point(0.57529, 0.53189),
                         Qt.point(0.41261, 0.59189),
                         Qt.point(0.53001, 0.32786),
                         Qt.point(0.47131, 0.32986)]
                       ]
            }
        }
    }
}
