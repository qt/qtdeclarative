/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

import QtQuick 2.2

Rectangle {
    width: 200
    height: 200
    color: "white"

    property alias left_animator: left_animator
    property alias right_animator: right_animator
    property alias rectangle: rect

    Rectangle {
        id: rect
        width: 100
        height: 200
        color: "red"

        state: "left"
        states: [
            State {
                name: "left"
            },
            State {
                name: "right"
            }
        ]

        transitions: [
            Transition {
                to: "left"

                XAnimator {
                    id: left_animator
                    target: rect
                    duration: 500
                    to: 0
                }
            },
            Transition {
                to: "right"

                XAnimator {
                    id: right_animator
                    target: rect
                    duration: 500
                    to: 100
                }
            }
        ]
    }
}
