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

import QtQuick

Rectangle {
    id: root
    width: 130
    height: 200
    property int widthFactor: 7

    Column {
        anchors.fill: parent
        Text {
            id: text1
            width: parent.width;
            text: qsTrId("TRID_1")
        }
        Text {
            id: text2
            width: 100;
            text: qsTrId("TRID_2")
        }
        Text {
            id: text3
            width: parent.width;
            text: qsTrId("TRID_3")
        }
        Text {
            id: text4
            width: parent.width;
            text: qsTrId("TRID_4")
        }
        Text {
            id: text5
            width: parent.width;
            text: qsTrId("TRID_5")
        }
        Text {
            id: text6
            width: parent.width;
            text: "way too long not translated text that should elide but not be marked"
        }
    }

    states: [
        State {
            name: "BiggerFontState"

            PropertyChanges {
                target: text1
                font.pointSize: 20
            }

            PropertyChanges {
                target: text2
                font.pointSize: 20
            }

            PropertyChanges {
                target: text3
                font.pointSize: 20
            }

        },
        State {
            name: "WayBiggerFontState"

            PropertyChanges {
                target: text1
                font.pointSize: 30
            }

            PropertyChanges {
                target: text2
                font.pointSize: 30
            }

            PropertyChanges {
                target: text3
                font.pointSize: 30
            }
        }
    ]
}
