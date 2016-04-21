/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

import QtQuick 2.6
import QtQuick.Window 2.0
import QtQuick.Controls 2.0

Window {
    width: 200
    height: 200
    visible: true

    Frame {
        padding: 0
        anchors.centerIn: parent
        width: row.implicitWidth
        height: row.implicitHeight

        FontMetrics {
            id: fontMetrics
        }

        Component {
            id: delegateComponent

            Text {
                text: modelData.toString().length < 2 ? "0" + modelData : modelData
                color: "#666666"
                opacity: 0.4 + Math.max(0, 1 - Math.abs(Tumbler.displacement)) * 0.6
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: fontMetrics.font.pixelSize * 1.25
            }
        }

        Row {
            id: row

            Tumbler {
                id: hoursTumbler
                model: 12
                delegate: Text {
                    text: (modelData.toString().length < 2 ? "0" : "") + (modelData + 1)
                    color: "#666666"
                    opacity: 0.4 + Math.max(0, 1 - Math.abs(Tumbler.displacement)) * 0.6
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: fontMetrics.font.pixelSize * 1.25
                }
                width: 50
                height: 150
            }

            Tumbler {
                id: minutesTumbler
                currentIndex: 6
                model: 60
                delegate: delegateComponent
                width: 50
                height: 150
            }

            Tumbler {
                id: amPmTumbler
                model: ["AM", "PM"]
                delegate: delegateComponent
                width: 50
                height: 150
                contentItem: ListView {
                    anchors.fill: parent
                    model: amPmTumbler.model
                    delegate: amPmTumbler.delegate

                    snapMode: ListView.SnapToItem
                    highlightRangeMode: ListView.StrictlyEnforceRange
                    preferredHighlightBegin: height / 2 - (height / 3 / 2)
                    preferredHighlightEnd: height / 2  + (height / 3 / 2)
                    clip: true
                }
            }
        }
    }
}
