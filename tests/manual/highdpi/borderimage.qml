/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Rectangle {
    width: 900
    height: 400

    readonly property real imageBorder: 32
    readonly property real animDuration: 3000
    readonly property real animMin: 2 * imageBorder
    readonly property real animMax: 280

    Text {
        anchors.bottom: row.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Green => standard DPI; purple => @2x"
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 10
        Repeater {
            model: 3
            delegate: Item {
                width: animMax
                height: animMax
                BorderImage {
                    source : index === 0 ? "BorderImage.png" : "TiledBorderImage.png"
                    anchors.centerIn: parent

                    border {
                        left: imageBorder; right: imageBorder
                        top: imageBorder; bottom: imageBorder
                    }

                    horizontalTileMode: index === 0 ? BorderImage.Stretch :
                                        index === 1 ? BorderImage.Repeat : BorderImage.Round
                    verticalTileMode: index === 0 ? BorderImage.Stretch :
                                      index === 1 ? BorderImage.Repeat : BorderImage.Round

                    width: animMin
                    SequentialAnimation on width {
                        NumberAnimation { to: animMax; duration: animDuration }
                        NumberAnimation { to: animMin; duration: animDuration }
                        loops: Animation.Infinite
                    }

                    height: animMax
                    SequentialAnimation on height {
                        NumberAnimation { to: animMin; duration: animDuration }
                        NumberAnimation { to: animMax; duration: animDuration }
                        loops: Animation.Infinite
                    }
                }

                Text {
                    anchors.top: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: 18
                    text: index === 0 ? "Stretch" :
                          index === 1 ? "Repeat" : "Round"
                }
            }
        }
    }
}
